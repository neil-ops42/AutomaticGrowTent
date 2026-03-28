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
      f.println(String("on_hour=") + d.on_hour);
      f.println(String("off_hour=") + d.off_hour);
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
    }
  }
  f.close();

  out = tmp;
  return true;
}

bool SettingsClass::save(const AppSettings &in) {
  if (!ensureFS()) return false;

  File f = LittleFS.open(SETTINGS_FILE, FILE_WRITE);
  if (!f) return false;

  f.println(String("mode=") + modeToStr(in.mode));
  f.println(String("on_hour=") + in.on_hour);
  f.println(String("off_hour=") + in.off_hour);
  f.close();
  return true;
}