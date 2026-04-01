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
    }
  }
  if (isnan(tmp.fan_on_temp_c)) tmp.fan_on_temp_c = FAN_ON_TEMP_C;
  if (isnan(tmp.fan_off_temp_c)) tmp.fan_off_temp_c = FAN_OFF_TEMP_C;
  if (tmp.fan_off_temp_c >= tmp.fan_on_temp_c) tmp.fan_off_temp_c = tmp.fan_on_temp_c - FAN_MIN_HYSTERESIS_C;
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
  f.close();
  return true;
}
