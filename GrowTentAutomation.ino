#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <time.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "secrets.h"
#include "sensors.h"
#include "relays.h"
#include "datalog.h"
#include "ui_oled.h"
#include "webserver.h"
#include "html_templates.h"
#include "settings.h"

const char* NTP_SERVER        = "ca.pool.ntp.org";
const char* HISTORY_FILE      = "/history.csv";
const char* HISTORY_OLD_FILE  = "/history_old.csv";
const char* SETTINGS_FILE     = "/settings.txt";

void connectWiFi();
void initTime();

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== GrowTentAutomation Starting ===");

    // Register Arduino task with watchdog
    esp_task_wdt_add(NULL);

    // 1. I2C (needed by sensors)
    Wire.begin(SDA_PIN, SCL_PIN);
    delay(10);

    // 2. WiFi
    connectWiFi();

    // 3. NTP Time
    initTime();

    // 4. GPIO setup
    pinMode(ONE_WIRE_BUS, INPUT_PULLUP);

    // 5. Storage (mounts LittleFS once)
    Settings.begin();

    // 6. Sensors (now Wire is ready)
    Sensors.begin();

    // 7. Other modules
    Relays.begin();
    DataLog.begin();
    OLED.begin();
    WebServer.begin();

    Serial.println("System initialization complete.\n");
}

void loop() {
    // WiFi reconnection check (every 30 seconds)
    static unsigned long lastWiFiCheck = 0;
    unsigned long now = millis();
    if (now - lastWiFiCheck >= 30000) {
        lastWiFiCheck = now;
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi lost — reconnecting...");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
    }

    Sensors.loop();
    esp_task_wdt_reset();
    Relays.loop();
    esp_task_wdt_reset();
    DataLog.loop();
    esp_task_wdt_reset();
    OLED.loop();
    esp_task_wdt_reset();
    WebServer.loop();

    // Handle deferred restart request (avoids delay() inside async handlers)
    if (WebServer.restartRequested) {
        delay(500);
        ESP.restart();
    }

    delay(2);
}

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