#include "ui_oled.h"
#include <WiFi.h>

OLEDClass OLED;

// ─────────────────────────────────────────────
//  WiFi Signal Icons (16×16)
// ─────────────────────────────────────────────

const unsigned char wifi_4bar[] PROGMEM = {
  0x00,0x00,0x03,0x80,0x07,0xC0,0x0E,0xE0,
  0x1C,0x70,0x38,0x38,0x70,0x1C,0xE0,0x0E,
  0xE0,0x0E,0xE0,0x0E,0xE0,0x0E,0xE0,0x0E,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const unsigned char wifi_3bar[] PROGMEM = {
  0x00,0x00,0x03,0x80,0x07,0xC0,0x0E,0xE0,
  0x1C,0x70,0x38,0x38,0x70,0x1C,0x70,0x1C,
  0x70,0x1C,0x70,0x1C,0x70,0x1C,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const unsigned char wifi_2bar[] PROGMEM = {
  0x00,0x00,0x03,0x80,0x07,0xC0,0x0E,0xE0,
  0x1C,0x70,0x38,0x38,0x38,0x38,0x38,0x38,
  0x38,0x38,0x38,0x38,0x38,0x38,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const unsigned char wifi_1bar[] PROGMEM = {
  0x00,0x00,0x03,0x80,0x07,0xC0,0x0E,0xE0,
  0x0E,0xE0,0x0E,0xE0,0x0E,0xE0,0x0E,0xE0,
  0x0E,0xE0,0x0E,0xE0,0x0E,0xE0,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const unsigned char wifi_0bar[] PROGMEM = {
  0x00,0x00,0x03,0x80,0x07,0xC0,0x07,0xC0,
  0x07,0xC0,0x07,0xC0,0x07,0xC0,0x07,0xC0,
  0x07,0xC0,0x07,0xC0,0x07,0xC0,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

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
//  Draw WiFi Signal Icon
// ─────────────────────────────────────────────
void OLEDClass::drawWiFiIcon(int rssi)
{
    if (rssi > WIFI_LVL_4)
        display.drawBitmap(112, 0, wifi_4bar, 16, 16, WHITE);
    else if (rssi > WIFI_LVL_3)
        display.drawBitmap(112, 0, wifi_3bar, 16, 16, WHITE);
    else if (rssi > WIFI_LVL_2)
        display.drawBitmap(112, 0, wifi_2bar, 16, 16, WHITE);
    else if (rssi > WIFI_LVL_1)
        display.drawBitmap(112, 0, wifi_1bar, 16, 16, WHITE);
    else
        display.drawBitmap(112, 0, wifi_0bar, 16, 16, WHITE);
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
    // WiFi icon
    drawWiFiIcon(WiFi.RSSI());

    SensorData s = Sensors.getData();
    RelayState r = Relays.getState();

    display.setCursor(0, 18);

    // Air Temp
    display.print("Air: ");
    if (isnan(s.airTemp)) display.println("ERR");
    else {
        display.print(s.airTemp, 1);
        display.println("C");
    }

    // Humidity
    display.print("Hum: ");
    if (isnan(s.airHum)) display.println("ERR");
    else {
        display.print(s.airHum, 1);
        display.println("%");
    }

    // VPD
    display.print("VPD: ");
    if (isnan(s.vpd)) display.println("ERR");
    else {
        display.print(s.vpd, 2);
        display.println("kPa");
    }

    // Water Temp
    display.print("Water: ");
    if (isnan(s.waterTemp)) display.println("ERR");
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