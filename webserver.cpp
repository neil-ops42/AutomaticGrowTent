#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include <esp_task_wdt.h>

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

static bool parseBoolStr(const String& v)
{
  String t = v;
  t.trim();
  t.toLowerCase();
  return (t == "1" || t == "true" || t == "yes" || t == "on");
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

  // Disable watchdog while OTA is in progress so large uploads
  // don't trigger a panic reboot mid-flash
  ElegantOTA.onStart([]() {
    Serial.println("OTA update starting - disabling watchdog");
    esp_task_wdt_delete(NULL);
  });

  // After OTA completes, cleanly tear down before restarting
  ElegantOTA.onEnd([this](bool success) {
    if (success) {
      Serial.println("OTA success - restarting cleanly");
      ws.closeAll();
      server.end();
      delay(500);
      ESP.restart();
    } else {
      Serial.println("OTA failed - re-enabling watchdog");
      esp_task_wdt_add(NULL);
    }
  });

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

    StaticJsonDocument<192> doc;
    doc["air_temp"]     = s.airTemp;   // ArduinoJson serializes NaN as null
    doc["air_humidity"] = s.airHum;
    doc["water_temp"]   = s.waterTemp;
    doc["vpd"]          = s.vpd;
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
    if (requireAuth(req)) return;
    bool ok = true;

    // Remove existing file
    if (LittleFS.exists(HISTORY_FILE)) {
        ok = LittleFS.remove(HISTORY_FILE);
    }

    // Recreate with CSV header that matches the logger
    // Header: time,airTemp,airHum,waterTemp,vpd,lightOn
    if (ok) {
        File f = LittleFS.open(HISTORY_FILE, FILE_WRITE);
        if (!f) ok = false;
        else {
          f.println("time,airTemp,airHum,waterTemp,vpd,lightOn"); // keep consistent with DataLog
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

    StaticJsonDocument<192> doc;
    doc["air_temp"]     = s.airTemp;   // ArduinoJson serializes NaN as null
    doc["air_humidity"] = s.airHum;
    doc["water_temp"]   = s.waterTemp;
    doc["vpd"]          = s.vpd;
    doc["time"]         = ts;

    String json;
    serializeJson(doc, json);
    req->send(200, "application/json", json);
  });

  // GET /api/device — ESP32 system metrics snapshot
  server.on("/api/device", HTTP_GET, [](AsyncWebServerRequest* req){
    uint32_t ramTotal   = ESP.getHeapSize();
    uint32_t ramFree    = ESP.getFreeHeap();
    uint32_t ramUsed    = ramTotal - ramFree;
    float    ramPct     = ramTotal > 0 ? (ramUsed * 100.0f / ramTotal) : 0.0f;

    uint64_t stTotal    = LittleFS.totalBytes();
    uint64_t stUsed     = LittleFS.usedBytes();
    uint64_t stFree     = stTotal - stUsed;
    float    stPct      = stTotal > 0 ? (stUsed * 100.0f / stTotal) : 0.0f;

    uint32_t skUsed     = ESP.getSketchSize();
    uint32_t skFree     = ESP.getFreeSketchSpace();
    uint32_t skTotal    = skUsed + skFree;
    float    skPct      = skTotal > 0 ? (skUsed * 100.0f / skTotal) : 0.0f;

    StaticJsonDocument<768> doc;
    doc["ram_total"]     = ramTotal;
    doc["ram_free"]      = ramFree;
    doc["ram_used"]      = ramUsed;
    doc["ram_pct"]       = ramPct;
    doc["storage_total"] = (uint32_t)stTotal;
    doc["storage_used"]  = (uint32_t)stUsed;
    doc["storage_free"]  = (uint32_t)stFree;
    doc["storage_pct"]   = stPct;
    doc["cpu_freq_mhz"]  = ESP.getCpuFreqMHz();
    doc["cpu_model"]     = ESP.getChipModel();
    doc["cpu_cores"]     = ESP.getChipCores();
    doc["flash_size"]    = ESP.getFlashChipSize();
    doc["sketch_used"]   = skUsed;
    doc["sketch_total"]  = skTotal;
    doc["sketch_pct"]    = skPct;
    doc["uptime_sec"]    = (uint32_t)(millis() / 1000UL);
    doc["wifi_rssi"]     = WiFi.RSSI();
    doc["wifi_ip"]       = WiFi.localIP().toString();
    doc["wifi_mac"]      = WiFi.macAddress();
    doc["sdk_version"]   = ESP.getSdkVersion();

    String json;
    serializeJson(doc, json);
    req->send(200, "application/json", json);
  });

  // Relay REST fallbacks — POST to avoid GET side-effects / CSRF
  server.on("/relay1/on",  HTTP_POST, [this](AsyncWebServerRequest* req){ if (requireAuth(req)) return; Relays.setRelay(1, true);  broadcastRelayState(); req->send(200,"text/plain","Light ON"); });
  server.on("/relay1/off", HTTP_POST, [this](AsyncWebServerRequest* req){ if (requireAuth(req)) return; Relays.setRelay(1, false); broadcastRelayState(); req->send(200,"text/plain","Light OFF"); });
  server.on("/relay2/on",  HTTP_POST, [this](AsyncWebServerRequest* req){ if (requireAuth(req)) return; Relays.setRelay(2, true);  broadcastRelayState(); req->send(200,"text/plain","Fan ON"); });
  server.on("/relay2/off", HTTP_POST, [this](AsyncWebServerRequest* req){ if (requireAuth(req)) return; Relays.setRelay(2, false); broadcastRelayState(); req->send(200,"text/plain","Fan OFF"); });

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
    Relays.state.mode = MODE_CUSTOM; // switch to custom first
    Relays.setCustomSchedule((uint8_t)onH, (uint8_t)offH); // this saves settings internally
    Relays.loop();                   // apply immediately
    WebServer.broadcastRelayState(); // notify clients

    req->send(200, "application/json", "{\"ok\":true}");
  });

  // POST /controls/config -> update full control config
  server.on("/controls/config", HTTP_POST, [](AsyncWebServerRequest* req){
    auto clampHour = [](int x) -> uint8_t {
      if (x < 0) x = 0;
      if (x > 23) x = 23;
      return (uint8_t)x;
    };

    ControlConfig c = Relays.getControlConfig();
    if (req->hasParam("custom_on", true))     c.customOnHour = clampHour(req->getParam("custom_on", true)->value().toInt());
    if (req->hasParam("custom_off", true))    c.customOffHour = clampHour(req->getParam("custom_off", true)->value().toInt());
    if (req->hasParam("veg_on", true))        c.vegOnHour = clampHour(req->getParam("veg_on", true)->value().toInt());
    if (req->hasParam("veg_off", true))       c.vegOffHour = clampHour(req->getParam("veg_off", true)->value().toInt());
    if (req->hasParam("flower_on", true))     c.flowerOnHour = clampHour(req->getParam("flower_on", true)->value().toInt());
    if (req->hasParam("flower_off", true))    c.flowerOffHour = clampHour(req->getParam("flower_off", true)->value().toInt());
    if (req->hasParam("auto_fan", true))      c.autoFan = parseBoolStr(req->getParam("auto_fan", true)->value());
    if (req->hasParam("fan_on_temp", true))   c.fanOnTempC = req->getParam("fan_on_temp", true)->value().toFloat();
    if (req->hasParam("fan_off_temp", true))  c.fanOffTempC = req->getParam("fan_off_temp", true)->value().toFloat();
    if (c.fanOffTempC >= c.fanOnTempC) c.fanOffTempC = c.fanOnTempC - FAN_MIN_HYSTERESIS_C;

    Relays.setControlConfig(c);
    Relays.loop();
    WebServer.broadcastRelayState();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  // GET /settings -> {"mode":"veg|flower|custom","on_hour":7,"off_hour":1}
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest* req){
    uint8_t onH, offH;
    Relays.getCustomSchedule(onH, offH);
    const char* modeStr = Relays.state.mode == MODE_VEG    ? "veg"
                        : Relays.state.mode == MODE_FLOWER ? "flower"
                                                           : "custom";
    StaticJsonDocument<256> doc;
    ControlConfig cfg = Relays.getControlConfig();
    doc["mode"]     = modeStr;
    doc["on_hour"]  = onH;
    doc["off_hour"] = offH;
    doc["veg_on_hour"] = cfg.vegOnHour;
    doc["veg_off_hour"] = cfg.vegOffHour;
    doc["flower_on_hour"] = cfg.flowerOnHour;
    doc["flower_off_hour"] = cfg.flowerOffHour;
    doc["auto_fan"] = cfg.autoFan;
    doc["fan_on_temp_c"] = cfg.fanOnTempC;
    doc["fan_off_temp_c"] = cfg.fanOffTempC;

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
  // NOTE: WebSocket connections are not authenticated. For sensitive operations
  // (settings reset, firmware update), use the authenticated HTTP endpoints instead.
  // WebSocket commands are convenience shortcuts for the local network UI.
  ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len){
    if (type == WS_EVT_CONNECT) {
      // Send current state directly to the newly connected client
      RelayState r = Relays.getState();
      uint8_t onH, offH;
      Relays.getCustomSchedule(onH, offH);
      const char* modeStr = r.mode == MODE_VEG    ? "veg"
                          : r.mode == MODE_FLOWER  ? "flower"
                                                   : "custom";
      StaticJsonDocument<256> doc;
      ControlConfig cfg = Relays.getControlConfig();
      doc["relay1"]          = r.light;
      doc["relay2"]          = r.fan;
      doc["mode"]            = modeStr;
      doc["on_hour"]         = onH;
      doc["off_hour"]        = offH;
      doc["veg_on_hour"]     = cfg.vegOnHour;
      doc["veg_off_hour"]    = cfg.vegOffHour;
      doc["flower_on_hour"]  = cfg.flowerOnHour;
      doc["flower_off_hour"] = cfg.flowerOffHour;
      doc["auto_fan"]        = cfg.autoFan;
      doc["fan_on_temp_c"]   = cfg.fanOnTempC;
      doc["fan_off_temp_c"]  = cfg.fanOffTempC;
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
        Relays.saveControlConfig();
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

  StaticJsonDocument<256> doc;
  ControlConfig cfg = Relays.getControlConfig();
  doc["relay1"]           = r.light;
  doc["relay2"]           = r.fan;
  doc["mode"]             = modeStr;
  doc["on_hour"]          = onH;
  doc["off_hour"]         = offH;
  doc["veg_on_hour"]      = cfg.vegOnHour;
  doc["veg_off_hour"]     = cfg.vegOffHour;
  doc["flower_on_hour"]   = cfg.flowerOnHour;
  doc["flower_off_hour"]  = cfg.flowerOffHour;
  doc["auto_fan"]         = cfg.autoFan;
  doc["fan_on_temp_c"]    = cfg.fanOnTempC;
  doc["fan_off_temp_c"]   = cfg.fanOffTempC;
  doc["manual_override"]  = r.manualLightOverride;

  String json;
  serializeJson(doc, json);
  ws.textAll(json);
}
