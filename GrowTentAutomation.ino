#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "config.h"
#include "sensors.h"
#include "relays.h"
#include "datalog.h"
#include "ui_oled.h"
#include "webserver.h"
#include "html_templates.h"
#include "settings.h"

// ─────────────────────────────────────────────
// REAL DEFINITIONS OF EXTERN VARIABLES
// (These MUST appear only once in the project)
// ─────────────────────────────────────────────
const char* WIFI_SSID     = "Wii";
const char* WIFI_PASSWORD = "netc42!#";

const char* NTP_SERVER    = "ca.pool.ntp.org";
const char* HISTORY_FILE  = "/history.csv";

// ─────────────────────────────────────────────
// Function Prototypes
// ─────────────────────────────────────────────
void connectWiFi();
void initTime();

// ─────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== GrowTentAutomation Starting ===");

    // WiFi
    connectWiFi();

    // NTP Time
    initTime();

    // Initialize all modules
    pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
    Settings.begin();
    Sensors.begin();
    Relays.begin();
    DataLog.begin();
    OLED.begin();
    WebServer.begin();

    Serial.println("System initialization complete.\n");
}

// ─────────────────────────────────────────────
// LOOP — NON-BLOCKING SYSTEM TICK
// ─────────────────────────────────────────────
void loop() {
    Sensors.loop();
    Relays.loop();
    DataLog.loop();
    OLED.loop();
    WebServer.loop();

    delay(2); // Keep system responsive without blocking
}

// ─────────────────────────────────────────────
// Connect to WiFi
// ─────────────────────────────────────────────
void connectWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 40) {
        delay(250);
        Serial.print(".");
        retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi connection FAILED.");
    }
}

// ─────────────────────────────────────────────
// Initialize NTP time
// ─────────────────────────────────────────────
void initTime() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("❌ Failed to obtain time from NTP.");
        return;
    }

    Serial.print("Current time: ");
    Serial.println(asctime(&timeinfo));
}