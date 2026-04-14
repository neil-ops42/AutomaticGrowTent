#ifndef UI_OLED_H
#define UI_OLED_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "config.h"
#include "sensors.h"
#include "relays.h"

class OLEDClass
{
public:
    void begin();
    void loop();

private:
    void render();

    unsigned long lastRefresh = 0;
    const unsigned long REFRESH_INTERVAL_MS = 1000;

    Adafruit_SSD1306 display =
        Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
};

extern OLEDClass OLED;

#endif