#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Adafruit_SHT4x.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

// Unified sensor structure
struct SensorData
{
    float airTemp = NAN;
    float airHum  = NAN;
    float waterTemp = NAN;
    unsigned long lastUpdate = 0;
};

class SensorsClass
{
public:
    void begin();
    void loop();
    SensorData getData();

private:
    void readAir();
    void readWater();

    // SHT4x
    Adafruit_SHT4x sht4;
    bool sht4_ok = false;

    // DS18B20
    OneWire oneWire = OneWire(ONE_WIRE_BUS);
    DallasTemperature dallas = DallasTemperature(&oneWire);

    // Timers
    unsigned long lastAirRead = 0;
    unsigned long lastWaterRead = 0;

    // Latest sensor values
    SensorData current;
};

extern SensorsClass Sensors;

#endif