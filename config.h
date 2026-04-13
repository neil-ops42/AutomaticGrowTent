#ifndef CONFIG_H
#define CONFIG_H

// ─────────────────────────────────────────────
// EXTERN STRING CONSTANTS (defined once in main.ino)
// These must NOT be defined in headers.
// ─────────────────────────────────────────────
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const char* NTP_SERVER;

// HTTP Basic Auth credentials (defined in secrets.h)
extern const char* WEB_AUTH_USERNAME;
extern const char* WEB_AUTH_PASSWORD;
extern const char* HISTORY_FILE;
extern const char* HISTORY_OLD_FILE;
extern const char* SETTINGS_FILE;

// ─────────────────────────────────────────────
// TIME CONFIG
// ─────────────────────────────────────────────
constexpr long GMT_OFFSET_SEC     = -18000;   // UTC-5 (EST)
constexpr int  DAYLIGHT_OFFSET_SEC = 3600;    // DST

// ─────────────────────────────────────────────
// RELAY PINS + LOGIC
// ─────────────────────────────────────────────
#define PIN_RELAY_1 27      // Light relay pin
#define PIN_RELAY_2 26      // Fan relay pin

#define RELAY_ACTIVE_STATE   HIGH
#define RELAY_INACTIVE_STATE LOW

// ─────────────────────────────────────────────
// LIGHTING SCHEDULE (DEFAULT FOR CUSTOM MODE)
// ─────────────────────────────────────────────
constexpr int LIGHT_ON_HOUR  = 7;   // 7:00 AM
constexpr int LIGHT_OFF_HOUR = 1;   // 1:00 AM
constexpr int VEG_ON_HOUR    = 6;   // 6:00 AM
constexpr int VEG_OFF_HOUR   = 0;   // 12:00 AM (midnight)
constexpr int FLOWER_ON_HOUR = 8;   // 8:00 AM
constexpr int FLOWER_OFF_HOUR= 20;  // 8:00 PM

// ─────────────────────────────────────────────
// SENSOR PINS
// ─────────────────────────────────────────────
#define ONE_WIRE_BUS 14
#define SDA_PIN 21
#define SCL_PIN 22

// ─────────────────────────────────────────────
// OLED DISPLAY SETTINGS
// ─────────────────────────────────────────────
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

// ─────────────────────────────────────────────
// LOGGING + SENSOR TIMING
// ─────────────────────────────────────────────
constexpr unsigned long LOG_INTERVAL_MS          = 60 * 1000;   // 1 minute
constexpr unsigned long AIR_SENSOR_INTERVAL_MS   = 5000;        // 5 sec
constexpr unsigned long WATER_SENSOR_INTERVAL_MS = 5000;        // 5 sec
constexpr size_t        MAX_LOG_BYTES            = 400UL * 1024UL; // 400 KB log size limit

// ─────────────────────────────────────────────
// WIFI SIGNAL THRESHOLDS (RSSI)
// ─────────────────────────────────────────────
#define WIFI_LVL_4 -55
#define WIFI_LVL_3 -67
#define WIFI_LVL_2 -70
#define WIFI_LVL_1 -80

// ─────────────────────────────────────────────
// FAN AUTO-CONTROL THRESHOLDS
// ─────────────────────────────────────────────
constexpr float FAN_ON_TEMP_C  = 28.0;   // Turn fan ON above this temp
constexpr float FAN_OFF_TEMP_C = 26.0;   // Turn fan OFF below this temp (hysteresis)
constexpr float FAN_MIN_HYSTERESIS_C = 0.5;

#endif
