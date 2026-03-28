#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <time.h>

#include "webserver.h"
#include "html_templates.h"
#include "settings.h"

WebServerClass WebServer;

static inline String jnum(float v) { return isnan(v) ? "null" : String(v, 1); }

// =====================================================================
// Server Setup & WebSocket Logic
// =====================================================================
void WebServerClass::begin() {
  // Don't mount FS here - DataLog.begin() already did it
  if (!SPIFFS.exists("/")) {
    Serial.println("SPIFFS not ready - ensure DataLog initialized first");
    // Continue anyway, routes will handle gracefully
  }

  setupRoutes();
  setupWebSocket();

  // ElegantOTA default UI at /update
  ElegantOTA.begin(&server);

  server.begin();
  Serial.println("Web Server Started.");
}

void WebServerClass::loop() {
  ElegantOTA.loop();

  // Periodically broadcast live sensor data on WS for the Charts page
  static unsigned long lastSensorPush = 0;
  unsigned long now = millis();
  if (now - lastSensorPush >= 2000) { // every 2 seconds
    lastSensorPush = now;

    SensorData s = Sensors.getData();
    struct tm t; char ts[32];
    if (getLocalTime(&t)) strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &t);
    else strcpy(ts, "NO_TIME");

    String json = "{";
    json += "\"air_temp\":"     + jnum(s.airTemp)  + ",";
    json += "\"air_humidity\":" + jnum(s.airHum)    + ",";
    json += "\"water_temp\":"   + jnum(s.waterTemp) + ",";
    json += "\"time\":\""       + String(ts) + "\"";
    json += "}";

    ws.textAll(json);
  }
}

static void sendPage(AsyncWebServerRequest* req, const __FlashStringHelper* content, bool needsChartJs = false)
{
  String out;
  out.reserve(4096);

  // 1) Header
  out += FPSTR(HTML_HEADER);

  // 2) Only add Chart.js for pages that need it
  if (needsChartJs) {
    out += "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>";
  }

  // 3) Page-specific content and footer
  out += FPSTR((PGM_P)content);
  out += FPSTR(HTML_FOOTER);

  // Send as UTF-8 to avoid degree-symbol issues
  req->send(200, "text/html; charset=utf-8", out);
}


void WebServerClass::setupRoutes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
    sendPage(req, (const __FlashStringHelper*)HTML_INDEX);
  });

  server.on("/controls", HTTP_GET, [](AsyncWebServerRequest* req){
    sendPage(req, (const __FlashStringHelper*)HTML_CONTROL);
  });

  server.on("/charts", HTTP_GET, [](AsyncWebServerRequest* req){
    sendPage(req, (const __FlashStringHelper*)HTML_CHARTS, /*needsChartJs=*/true);
  });

  server.on("/history", HTTP_GET, [](AsyncWebServerRequest* req){
    sendPage(req, (const __FlashStringHelper*)HTML_HISTORY, /*needsChartJs=*/true);
  });

  // CSV history passthrough
  server.on("/history.csv", HTTP_GET, [](AsyncWebServerRequest* req){
    req->send(SPIFFS, HISTORY_FILE, "text/csv");
  });
  
  server.on("/history/reset", HTTP_POST, [](AsyncWebServerRequest* req){
    bool ok = true;

    // Remove existing file
    if (SPIFFS.exists(HISTORY_FILE)) {
        ok = SPIFFS.remove(HISTORY_FILE);
    }

    // Recreate with CSV header that matches the logger
    // Header: time,airTemp,airHum,waterTemp
    if (ok) {
        File f = SPIFFS.open(HISTORY_FILE, FILE_WRITE);
        if (!f) ok = false;
        else {
        f.println("time,airTemp,airHum,waterTemp"); // keep consistent with DataLog
        f.close();
        }
    }

    req->send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });

  // JSON sensor snapshot (/data)
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest* req){
    SensorData s = Sensors.getData();
    struct tm t;
    char ts[32];
    if (getLocalTime(&t)) strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &t);
    else strcpy(ts, "NO_TIME");

    String json = "{";
    json += "\"air_temp\":" + jnum(s.airTemp) + ",";
    json += "\"air_humidity\":" + jnum(s.airHum) + ",";
    json += "\"water_temp\":" + jnum(s.waterTemp) + ",";
    json += "\"time\":\"" + String(ts) + "\"";
    json += "}";

    req->send(200, "application/json", json);
  });

  // Relay REST fallbacks (if you click these from outside Controls)
  server.on("/relay1/on",  HTTP_GET, [this](AsyncWebServerRequest* req){ Relays.setRelay(1, true);  broadcastRelayState(); req->send(200,"text/plain","Light ON"); });
  server.on("/relay1/off", HTTP_GET, [this](AsyncWebServerRequest* req){ Relays.setRelay(1, false); broadcastRelayState(); req->send(200,"text/plain","Light OFF"); });
  server.on("/relay2/on",  HTTP_GET, [this](AsyncWebServerRequest* req){ Relays.setRelay(2, true);  broadcastRelayState(); req->send(200,"text/plain","Fan ON"); });
  server.on("/relay2/off", HTTP_GET, [this](AsyncWebServerRequest* req){ Relays.setRelay(2, false); broadcastRelayState(); req->send(200,"text/plain","Fan OFF"); });

  // GET /schedule -> {"mode":"veg|flower|custom","on_hour":7,"off_hour":1}
  server.on("/schedule", HTTP_GET, [](AsyncWebServerRequest* req){
    uint8_t onH, offH; Relays.getCustomSchedule(onH, offH);
    RelayState r = Relays.getState();
    String json = "{";
    json += "\"mode\":\"" + String(r.mode==MODE_VEG ? "veg" : r.mode==MODE_FLOWER ? "flower" : "custom") + "\",";
    json += "\"on_hour\":" + String(onH) + ",";
    json += "\"off_hour\":" + String(offH);
    json += "}";
    req->send(200, "application/json", json);
  });

  // POST /schedule => on=HH&off=HH
  server.on("/schedule", HTTP_POST, [](AsyncWebServerRequest* req){
    if (!req->hasParam("on", true) || !req->hasParam("off", true)) {
      req->send(400, "application/json", "{\"error\":\"on/off required\"}");
      return;
    }
    int onH = req->getParam("on", true)->value().toInt();
    int offH = req->getParam("off", true)->value().toInt();
    if (onH < 0) onH = 0; if (onH > 23) onH = 23;
    if (offH < 0) offH = 0; if (offH > 23) offH = 23;
    Relays.setCustomSchedule((uint8_t)onH, (uint8_t)offH);
    Relays.state.mode = MODE_CUSTOM; // switch to custom
    Relays.loop();                   // apply immediately
    WebServer.broadcastRelayState(); // notify clients
    AppSettings s;
s.mode = MODE_CUSTOM;
s.on_hour = (uint8_t)onH;
s.off_hour = (uint8_t)offH;
Settings.save(s);

    req->send(200, "application/json", "{\"ok\":true}");
  });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest* req){
    uint8_t onH, offH; Relays.getCustomSchedule(onH, offH);
    String mode = (Relays.state.mode==MODE_VEG?"veg": Relays.state.mode==MODE_FLOWER?"flower":"custom");
    String json = "{";
    json += "\"mode\":\"" + mode + "\",";
    json += "\"on_hour\":" + String(onH) + ",";
    json += "\"off_hour\":" + String(offH);
    json += "}";
    req->send(200, "application/json", json);
    });

  // NOTE: /update is handled by ElegantOTA's own handler (default UI).
}

void WebServerClass::setupWebSocket() {
  ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len){
    if (type == WS_EVT_CONNECT) {
      broadcastRelayState(); // navbar + controls get initial state
      return;
    }
    if (type == WS_EVT_DATA) {
      String msg;
      msg.reserve(len + 1);
      for (size_t i = 0; i < len; i++) msg += (char)data[i];

      if (msg == "relay1_on")  Relays.setRelay(1, true);
      if (msg == "relay1_off") Relays.setRelay(1, false);
      if (msg == "relay2_on")  Relays.setRelay(2, true);
      if (msg == "relay2_off") Relays.setRelay(2, false);

      if (msg == "mode_veg" || msg == "mode_flower" || msg == "mode_custom") {
        if (msg == "mode_veg")    Relays.state.mode = MODE_VEG;
        if (msg == "mode_flower") Relays.state.mode = MODE_FLOWER;
        if (msg == "mode_custom") Relays.state.mode = MODE_CUSTOM;
        // Persist mode change (schedule changes are saved inside setCustomSchedule)
        uint8_t onH, offH; Relays.getCustomSchedule(onH, offH);
        AppSettings s; s.mode = Relays.state.mode; s.on_hour = onH; s.off_hour = offH;
        Settings.save(s);
      }

      broadcastRelayState();
    }
  });
  server.addHandler(&ws);
}

void WebServerClass::broadcastRelayState() {
  RelayState r = Relays.getState();
  uint8_t onH, offH; Relays.getCustomSchedule(onH, offH);

  String json = "{";
  json += "\"relay1\":" + String(r.light ? "true" : "false") + ",";
  json += "\"relay2\":" + String(r.fan   ? "true" : "false") + ",";
  json += "\"mode\":\"" + String(r.mode==MODE_VEG ? "veg" : r.mode==MODE_FLOWER ? "flower" : "custom") + "\",";
  json += "\"on_hour\":" + String(onH) + ",";
  json += "\"off_hour\":" + String(offH);
  json += "}";

  ws.textAll(json);
}
