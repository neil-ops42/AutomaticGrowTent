#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>

#include "webserver.h"
#include "html_templates.h"
#include "settings.h"

WebServerClass WebServer;

// ─────────────────────────────────────────────
// Helper: require HTTP Basic Auth on sensitive routes.
// Returns true and sends 401 if credentials are wrong.
// ─────────────────────────────────────────────
static bool requireAuth(AsyncWebServerRequest* req)
{
    if (!req->authenticate(WEB_AUTH_USERNAME, WEB_AUTH_PASSWORD)) {
        req->requestAuthentication();
        return true;
    }
    return false;
}

// =====================================================================
// Server Setup & WebSocket Logic
// =====================================================================
void WebServerClass::begin() {
  File root = LittleFS.open("/");
  if (!root) {
    Serial.println("LittleFS not ready - ensure Settings initialized first");
  } else {
    root.close();
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

    StaticJsonDocument<128> doc;
    doc["air_temp"]     = s.airTemp;   // ArduinoJson serializes NaN as null
    doc["air_humidity"] = s.airHum;
    doc["water_temp"]   = s.waterTemp;
    doc["time"]         = ts;

    String json;
    serializeJson(doc, json);
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
    sendPage(req, (const __FlashStringHelper*)HTML_INDEX, /*needsChartJs=*/true);
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
    req->send(LittleFS, HISTORY_FILE, "text/csv");
  });

  server.on("/settings/reset", HTTP_POST, [](AsyncWebServerRequest* req){
    if (requireAuth(req)) return;
    if (LittleFS.exists(SETTINGS_FILE)) {
      LittleFS.remove(SETTINGS_FILE);
    }
    req->send(200, "application/json", "{\"ok\":true}");
    WebServer.restartRequested = true;
  });

  server.on("/history/reset", HTTP_POST, [](AsyncWebServerRequest* req){
    bool ok = true;

    // Remove existing file
    if (LittleFS.exists(HISTORY_FILE)) {
        ok = LittleFS.remove(HISTORY_FILE);
    }

    // Recreate with CSV header that matches the logger
    // Header: time,airTemp,airHum,waterTemp
    if (ok) {
        File f = LittleFS.open(HISTORY_FILE, FILE_WRITE);
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

    StaticJsonDocument<128> doc;
    doc["air_temp"]     = s.airTemp;   // ArduinoJson serializes NaN as null
    doc["air_humidity"] = s.airHum;
    doc["water_temp"]   = s.waterTemp;
    doc["time"]         = ts;

    String json;
    serializeJson(doc, json);
    req->send(200, "application/json", json);
  });

  // Relay REST fallbacks — POST to avoid GET side-effects / CSRF
  server.on("/relay1/on",  HTTP_POST, [this](AsyncWebServerRequest* req){ Relays.setRelay(1, true);  broadcastRelayState(); req->send(200,"text/plain","Light ON"); });
  server.on("/relay1/off", HTTP_POST, [this](AsyncWebServerRequest* req){ Relays.setRelay(1, false); broadcastRelayState(); req->send(200,"text/plain","Light OFF"); });
  server.on("/relay2/on",  HTTP_POST, [this](AsyncWebServerRequest* req){ Relays.setRelay(2, true);  broadcastRelayState(); req->send(200,"text/plain","Fan ON"); });
  server.on("/relay2/off", HTTP_POST, [this](AsyncWebServerRequest* req){ Relays.setRelay(2, false); broadcastRelayState(); req->send(200,"text/plain","Fan OFF"); });

  // POST /schedule => on=HH&off=HH
  server.on("/schedule", HTTP_POST, [](AsyncWebServerRequest* req){
    if (!req->hasParam("on", true) || !req->hasParam("off", true)) {
      req->send(400, "application/json", "{\"error\":\"on/off required\"}");
      return;
    }
    int onH  = req->getParam("on",  true)->value().toInt();
    int offH = req->getParam("off", true)->value().toInt();
    if (onH  < 0) onH  = 0; if (onH  > 23) onH  = 23;
    if (offH < 0) offH = 0; if (offH > 23) offH = 23;
    Relays.setCustomSchedule((uint8_t)onH, (uint8_t)offH);
    Relays.state.mode = MODE_CUSTOM; // switch to custom
    Relays.loop();                   // apply immediately
    WebServer.broadcastRelayState(); // notify clients
    AppSettings s;
    s.mode     = MODE_CUSTOM;
    s.on_hour  = (uint8_t)onH;
    s.off_hour = (uint8_t)offH;
    Settings.save(s);

    req->send(200, "application/json", "{\"ok\":true}");
  });

  // GET /settings -> {"mode":"veg|flower|custom","on_hour":7,"off_hour":1}
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest* req){
    uint8_t onH, offH;
    Relays.getCustomSchedule(onH, offH);
    const char* modeStr = Relays.state.mode == MODE_VEG    ? "veg"
                        : Relays.state.mode == MODE_FLOWER ? "flower"
                                                           : "custom";
    StaticJsonDocument<64> doc;
    doc["mode"]     = modeStr;
    doc["on_hour"]  = onH;
    doc["off_hour"] = offH;

    String json;
    serializeJson(doc, json);
    req->send(200, "application/json", json);
  });

  // NOTE: /update is handled by ElegantOTA's own handler (default UI).

  // GET /settings/backup — download current settings as a file attachment
  server.on("/settings/backup", HTTP_GET, [](AsyncWebServerRequest* req){
    if (requireAuth(req)) return;
    if (!LittleFS.exists(SETTINGS_FILE)) {
      req->send(404, "application/json", "{\"error\":\"settings file not found\"}");
      return;
    }
    AsyncWebServerResponse* resp = req->beginResponse(LittleFS, SETTINGS_FILE, "text/plain");
    resp->addHeader("Content-Disposition", "attachment; filename=\"settings_backup.txt\"");
    req->send(resp);
  });

  // POST /settings/restore — upload a settings file, write it to LittleFS, and reload
  server.on("/settings/restore", HTTP_POST,
    [](AsyncWebServerRequest* req) {
      if (requireAuth(req)) return;
      AppSettings s;
      if (!Settings.load(s)) {
        req->send(500, "application/json", "{\"error\":\"settings saved but could not be parsed\"}");
        return;
      }
      Relays.state.mode = s.mode;
      Relays.setCustomSchedule(s.on_hour, s.off_hour);
      Relays.loop();
      req->send(200, "application/json", "{\"ok\":true}");
    },
    [](AsyncWebServerRequest* req, const String& /*filename*/, size_t index, uint8_t* data, size_t len, bool final) {
      if (index == 0) {
        req->_tempFile = LittleFS.open(SETTINGS_FILE, FILE_WRITE);
      }
      if (req->_tempFile) {
        req->_tempFile.write(data, len);
      }
      if (final && req->_tempFile) {
        req->_tempFile.close();
      }
    });

  // GET /history_old.csv — serve the archived log created by log rotation
  server.on("/history_old.csv", HTTP_GET, [](AsyncWebServerRequest* req){
    if (!LittleFS.exists(HISTORY_OLD_FILE)) {
      req->send(404, "application/json", "{\"error\":\"no archived log available\"}");
      return;
    }
    req->send(LittleFS, HISTORY_OLD_FILE, "text/csv");
  });
}

void WebServerClass::setupWebSocket() {
  ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len){
    if (type == WS_EVT_CONNECT) {
      // Send current state directly to the newly connected client
      RelayState r = Relays.getState();
      uint8_t onH, offH;
      Relays.getCustomSchedule(onH, offH);
      const char* modeStr = r.mode == MODE_VEG    ? "veg"
                          : r.mode == MODE_FLOWER  ? "flower"
                                                   : "custom";
      StaticJsonDocument<128> doc;
      doc["relay1"]          = r.light;
      doc["relay2"]          = r.fan;
      doc["mode"]            = modeStr;
      doc["on_hour"]         = onH;
      doc["off_hour"]        = offH;
      doc["manual_override"] = r.manualLightOverride;
      String json;
      serializeJson(doc, json);
      client->text(json);
      return;
    }
    if (type == WS_EVT_DATA) {
      String msg;
      msg.reserve(len + 1);
      for (size_t i = 0; i < len; i++) msg += (char)data[i];

      // Require explicit confirmation command before restarting to prevent
      // accidental or malicious reboots from a casual WebSocket message.
      if (msg == "device_restart_confirm") {
        restartRequested = true;  // handled safely in main loop()
      }

      if (msg == "relay1_on")  Relays.setRelay(1, true);
      if (msg == "relay1_off") Relays.setRelay(1, false);
      if (msg == "relay2_on")  Relays.setRelay(2, true);
      if (msg == "relay2_off") Relays.setRelay(2, false);

      if (msg == "schedule_resume") Relays.resumeSchedule();

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
  uint8_t onH, offH;
  Relays.getCustomSchedule(onH, offH);

  const char* modeStr = r.mode == MODE_VEG    ? "veg"
                      : r.mode == MODE_FLOWER  ? "flower"
                                               : "custom";

  StaticJsonDocument<128> doc;
  doc["relay1"]           = r.light;
  doc["relay2"]           = r.fan;
  doc["mode"]             = modeStr;
  doc["on_hour"]          = onH;
  doc["off_hour"]         = offH;
  doc["manual_override"]  = r.manualLightOverride;

  String json;
  serializeJson(doc, json);
  ws.textAll(json);
}
