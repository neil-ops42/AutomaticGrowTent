// settings.cpp
#include "settings.h"

SettingsClass Settings;

static String trimLower(const String& s) {
  String t = s;
  t.trim();
  t.toLowerCase();
  return t;
}

const char* SettingsClass::modeToStr(GrowMode m) {
  switch (m) {
    case MODE_VEG:    return "veg";
    case MODE_FLOWER: return "flower";
    case MODE_CUSTOM: default: return "custom";
  }
}

GrowMode SettingsClass::strToMode(const String& s) {
  String t = trimLower(s);
  if (t == "veg")    return MODE_VEG;
  if (t == "flower") return MODE_FLOWER;
  return MODE_CUSTOM;
}

bool SettingsClass::ensureFS() {
  // Mount FS (safe to call multiple times)
  return LittleFS.begin(true);
}

void SettingsClass::begin() {
  if (!ensureFS()) return;

  if (!LittleFS.exists(SETTINGS_FILE)) {
    // Create file with defaults from config.h
    File f = LittleFS.open(SETTINGS_FILE, FILE_WRITE);
    if (f) {
      AppSettings d;
      f.println(String("mode=") + modeToStr(d.mode));
      f.println(String("on_hour=") + String((int)d.on_hour));
      f.println(String("off_hour=") + String((int)d.off_hour));
      f.println(String("veg_on_hour=") + String((int)d.veg_on_hour));
      f.println(String("veg_off_hour=") + String((int)d.veg_off_hour));
      f.println(String("flower_on_hour=") + String((int)d.flower_on_hour));
      f.println(String("flower_off_hour=") + String((int)d.flower_off_hour));
      f.println(String("auto_fan=") + String(d.auto_fan ? 1 : 0));
      f.println(String("fan_on_temp_c=") + String(d.fan_on_temp_c, 2));
      f.println(String("fan_off_temp_c=") + String(d.fan_off_temp_c, 2));
      f.println(String("gmt_offset_sec=") + String(d.gmt_offset_sec));
      f.println(String("daylight_offset_sec=") + String(d.daylight_offset_sec));
      f.println(String("ntp_server=") + String(d.ntp_server));
      f.println(String("air_sensor_interval_sec=") + String(d.air_sensor_interval_sec));
      f.println(String("water_sensor_interval_sec=") + String(d.water_sensor_interval_sec));
      f.println(String("log_interval_sec=") + String(d.log_interval_sec));
      f.println(String("max_log_kb=") + String(d.max_log_kb));
      f.println(String("fan_min_hysteresis_c=") + String(d.fan_min_hysteresis_c, 2));
      f.println(String("fan_debounce_sec=") + String(d.fan_debounce_sec));
      f.close();
    }
  }
}

bool SettingsClass::load(AppSettings &out) {
  if (!ensureFS()) return false;
  if (!LittleFS.exists(SETTINGS_FILE)) return false;

  File f = LittleFS.open(SETTINGS_FILE, FILE_READ);
  if (!f) return false;

  // defaults before parse
  AppSettings tmp;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("#") || line.startsWith(";")) continue;
    int eq = line.indexOf('=');
    if (eq < 0) continue;

    String k = line.substring(0, eq);   k.trim(); k.toLowerCase();
    String v = line.substring(eq + 1);  v.trim();

    if (k == "mode") {
      tmp.mode = strToMode(v);
    } else if (k == "on_hour") {
      int x = v.toInt(); if (x < 0) x = 0; if (x > 23) x = 23;
      tmp.on_hour = (uint8_t)x;
    } else if (k == "off_hour") {
      int x = v.toInt(); if (x < 0) x = 0; if (x > 23) x = 23;
      tmp.off_hour = (uint8_t)x;
    } else if (k == "veg_on_hour") {
      int x = v.toInt(); if (x < 0) x = 0; if (x > 23) x = 23;
      tmp.veg_on_hour = (uint8_t)x;
    } else if (k == "veg_off_hour") {
      int x = v.toInt(); if (x < 0) x = 0; if (x > 23) x = 23;
      tmp.veg_off_hour = (uint8_t)x;
    } else if (k == "flower_on_hour") {
      int x = v.toInt(); if (x < 0) x = 0; if (x > 23) x = 23;
      tmp.flower_on_hour = (uint8_t)x;
    } else if (k == "flower_off_hour") {
      int x = v.toInt(); if (x < 0) x = 0; if (x > 23) x = 23;
      tmp.flower_off_hour = (uint8_t)x;
    } else if (k == "auto_fan") {
      String t = trimLower(v);
      tmp.auto_fan = (t == "1" || t == "true" || t == "yes" || t == "on");
    } else if (k == "fan_on_temp_c") {
      tmp.fan_on_temp_c = v.toFloat();
    } else if (k == "fan_off_temp_c") {
      tmp.fan_off_temp_c = v.toFloat();
    } else if (k == "gmt_offset_sec") {
      tmp.gmt_offset_sec = v.toInt();
    } else if (k == "daylight_offset_sec") {
      tmp.daylight_offset_sec = v.toInt();
    } else if (k == "ntp_server") {
      v.trim();
      if (v.length() > 0 && v.length() < sizeof(tmp.ntp_server)) {
        strncpy(tmp.ntp_server, v.c_str(), sizeof(tmp.ntp_server) - 1);
        tmp.ntp_server[sizeof(tmp.ntp_server) - 1] = '\0';
      }
    } else if (k == "air_sensor_interval_sec") {
      int x = v.toInt(); if (x < 1) x = 1; if (x > 3600) x = 3600;
      tmp.air_sensor_interval_sec = (uint16_t)x;
    } else if (k == "water_sensor_interval_sec") {
      int x = v.toInt(); if (x < 1) x = 1; if (x > 3600) x = 3600;
      tmp.water_sensor_interval_sec = (uint16_t)x;
    } else if (k == "log_interval_sec") {
      int x = v.toInt(); if (x < 10) x = 10; if (x > 3600) x = 3600;
      tmp.log_interval_sec = (uint16_t)x;
    } else if (k == "max_log_kb") {
      int x = v.toInt(); if (x < 10) x = 10; if (x > 4096) x = 4096;
      tmp.max_log_kb = (uint16_t)x;
    } else if (k == "fan_min_hysteresis_c") {
      float x = v.toFloat();
      if (!isnan(x) && x >= 0.1f && x <= 10.0f) tmp.fan_min_hysteresis_c = x;
    } else if (k == "fan_debounce_sec") {
      int x = v.toInt(); if (x < 0) x = 0; if (x > 600) x = 600;
      tmp.fan_debounce_sec = (uint16_t)x;
    }
  }
  if (isnan(tmp.fan_on_temp_c)) tmp.fan_on_temp_c = FAN_ON_TEMP_C;
  if (isnan(tmp.fan_off_temp_c)) tmp.fan_off_temp_c = FAN_OFF_TEMP_C;
  if (tmp.fan_off_temp_c >= tmp.fan_on_temp_c) tmp.fan_off_temp_c = tmp.fan_on_temp_c - tmp.fan_min_hysteresis_c;
  f.close();

  out = tmp;
  return true;
}

bool SettingsClass::save(const AppSettings &in) {
  if (!ensureFS()) return false;

  File f = LittleFS.open(SETTINGS_FILE, FILE_WRITE);
  if (!f) return false;

  f.println(String("mode=") + modeToStr(in.mode));
  f.println(String("on_hour=") + String((int)in.on_hour));
  f.println(String("off_hour=") + String((int)in.off_hour));
  f.println(String("veg_on_hour=") + String((int)in.veg_on_hour));
  f.println(String("veg_off_hour=") + String((int)in.veg_off_hour));
  f.println(String("flower_on_hour=") + String((int)in.flower_on_hour));
  f.println(String("flower_off_hour=") + String((int)in.flower_off_hour));
  f.println(String("auto_fan=") + String(in.auto_fan ? 1 : 0));
  f.println(String("fan_on_temp_c=") + String(in.fan_on_temp_c, 2));
  f.println(String("fan_off_temp_c=") + String(in.fan_off_temp_c, 2));
  f.println(String("gmt_offset_sec=") + String(in.gmt_offset_sec));
  f.println(String("daylight_offset_sec=") + String(in.daylight_offset_sec));
  f.println(String("ntp_server=") + String(in.ntp_server));
  f.println(String("air_sensor_interval_sec=") + String(in.air_sensor_interval_sec));
  f.println(String("water_sensor_interval_sec=") + String(in.water_sensor_interval_sec));
  f.println(String("log_interval_sec=") + String(in.log_interval_sec));
  f.println(String("max_log_kb=") + String(in.max_log_kb));
  f.println(String("fan_min_hysteresis_c=") + String(in.fan_min_hysteresis_c, 2));
  f.println(String("fan_debounce_sec=") + String(in.fan_debounce_sec));
  f.close();
  return true;
}
