// settings.h
#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include "relays.h"   // for GrowMode
#include "config.h"   // for LIGHT_ON_HOUR / LIGHT_OFF_HOUR

struct AppSettings {
  GrowMode mode = MODE_CUSTOM;
  uint8_t on_hour  = LIGHT_ON_HOUR;  // defaults from config.h
  uint8_t off_hour = LIGHT_OFF_HOUR; // defaults from config.h
  uint8_t veg_on_hour = VEG_ON_HOUR;
  uint8_t veg_off_hour = VEG_OFF_HOUR;
  uint8_t flower_on_hour = FLOWER_ON_HOUR;
  uint8_t flower_off_hour = FLOWER_OFF_HOUR;
  bool auto_fan = true;
  float fan_on_temp_c = FAN_ON_TEMP_C;
  float fan_off_temp_c = FAN_OFF_TEMP_C;
};

class SettingsClass {
public:
  void begin();                   // ensure FS is mounted and file exists (or create with defaults)
  bool load(AppSettings &out);    // returns true if file parsed successfully
  bool save(const AppSettings &in);

  // convenience helpers
  static const char* modeToStr(GrowMode m);
  static GrowMode strToMode(const String& s);

private:
  bool ensureFS();
};

extern SettingsClass Settings;

#endif
