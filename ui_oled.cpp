#include "ui_oled.h"
#include <WiFi.h>

OLEDClass OLED;

// ─────────────────────────────────────────────
//  Initialize OLED
// ─────────────────────────────────────────────
void OLEDClass::begin()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    {
        Serial.println("OLED init failed!");
        return;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("GreenAutomaton v1.0");
    display.display();

    delay(800);
}

// ─────────────────────────────────────────────
//  Non‑blocking update loop
// ─────────────────────────────────────────────
void OLEDClass::loop()
{
    unsigned long now = millis();
    if (now - lastRefresh < REFRESH_INTERVAL_MS)
        return;

    lastRefresh = now;
    render();
}

// ─────────────────────────────────────────────
//  Main Dashboard Renderer
// ─────────────────────────────────────────────
void OLEDClass::render()
{
    display.clearDisplay();
    display.setCursor(0, 0);

    // IP Address
    display.print(WiFi.localIP());

    SensorData s = Sensors.getData();
    RelayState r = Relays.getState();
    unsigned long now = millis();

    // Helper: format ERR age string into a small buffer (e.g. "ERR 2m")
    // Returns the buffer pointer for chaining
    auto errAge = [&](unsigned long lastValid, char* buf, size_t bufsz) -> const char* {
        if (lastValid == 0) {
            snprintf(buf, bufsz, "ERR");
        } else {
            unsigned long ageSec = (now - lastValid) / 1000UL;
            if (ageSec < 60) snprintf(buf, bufsz, "ERR %lus", ageSec);
            else             snprintf(buf, bufsz, "ERR %lum", ageSec / 60UL);
        }
        return buf;
    };

    char errBuf[12];

    display.setCursor(0, 16);

    // Air Temp
    display.print("Air: ");
    if (isnan(s.airTemp)) display.println(errAge(s.lastValidAirRead, errBuf, sizeof(errBuf)));
    else {
        display.print(s.airTemp, 1);
        display.println("C");
    }

    // Humidity
    display.print("Hum: ");
    if (isnan(s.airHum)) display.println(errAge(s.lastValidAirRead, errBuf, sizeof(errBuf)));
    else {
        display.print(s.airHum, 1);
        display.println("%");
    }

    // VPD
    display.print("VPD: ");
    if (isnan(s.vpd)) display.println(errAge(s.lastValidAirRead, errBuf, sizeof(errBuf)));
    else {
        display.print(s.vpd, 2);
        display.println("kPa");
    }

    // Water Temp
    display.print("Water: ");
    if (isnan(s.waterTemp)) display.println(errAge(s.lastValidWaterRead, errBuf, sizeof(errBuf)));
    else {
        display.print(s.waterTemp, 1);
        display.println("C");
    }

    // Light Relay
    display.print("Light: ");
    display.println(r.light ? "ON" : "OFF");

    // Fan Relay
    display.print("Fan:   ");
    display.println(r.fan ? "ON" : "OFF");

    display.display();
}