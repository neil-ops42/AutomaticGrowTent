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

    // Register Arduino task with watchdog (ESP-IDF v5 API)
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 30000,       // 30 second timeout
        .idle_core_mask = 0,       // don't watch idle tasks
        .trigger_panic = true      // panic (reboot) on timeout
    };
    esp_err_t wdt_err = esp_task_wdt_init(&wdt_config);
    if (wdt_err == ESP_ERR_INVALID_STATE) {
        // WDT already initialised by the framework — reconfigure it
        Serial.println("WDT already initialised — reconfiguring");
        wdt_err = esp_task_wdt_reconfigure(&wdt_config);
    }
    if (wdt_err != ESP_OK) {
        Serial.printf("⚠️  WDT init/reconfigure failed: %d\n", (int)wdt_err);
    }
    wdt_err = esp_task_wdt_add(NULL);
    if (wdt_err != ESP_OK && wdt_err != ESP_ERR_INVALID_ARG) {
        // ESP_ERR_INVALID_ARG means the task was already registered — that's fine
        Serial.printf("⚠️  WDT add task failed: %d\n", (int)wdt_err);
    }

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
    static unsigned long lastWiFiCheck = millis();
    static bool wifiReconnecting = false;
    static unsigned long wifiReconnectStart = 0;
    unsigned long now = millis();
    if (now - lastWiFiCheck >= 30000) {
        lastWiFiCheck = now;
        if (WiFi.status() == WL_CONNECTED) {
            if (wifiReconnecting) {
                // Just reconnected — re-sync NTP immediately
                wifiReconnecting = false;
                Serial.println("WiFi reconnected — re-syncing NTP");
                configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
            }
        } else if (!wifiReconnecting) {
            // Start a new reconnect attempt
            Serial.println("WiFi lost — reconnecting...");
            wifiReconnecting = true;
            wifiReconnectStart = now;
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        } else if (now - wifiReconnectStart >= 60000) {
            // Reconnect timeout — tear down and retry
            Serial.println("WiFi reconnect timeout — retrying...");
            wifiReconnecting = false;
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            wifiReconnecting = true;
            wifiReconnectStart = now;
        }
        // else: reconnect still in progress — do not interrupt it
    }

    // Periodic NTP re-sync check (every 60 seconds)
    static unsigned long lastNtpCheck = 0;
    if (now - lastNtpCheck >= 60000) {
        lastNtpCheck = now;
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo, 1000)) {
            Serial.println("NTP not synced — retrying...");
            configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
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
    esp_task_wdt_reset();

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