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
    float vpd = NAN;
    unsigned long lastUpdate = 0;
    unsigned long lastValidAirRead = 0;    // millis() of last non-NaN air reading
    unsigned long lastValidWaterRead = 0;  // millis() of last non-NaN water reading
};

class SensorsClass
{
public:
    void begin();
    void loop();
    SensorData getData();
    static float calcVPD(float tempC, float rh);

    // Runtime-configurable intervals (in milliseconds)
    void setAirInterval(unsigned long ms)   { airIntervalMs = ms; }
    void setWaterInterval(unsigned long ms)  { waterIntervalMs = ms; }

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

    // Configurable intervals (default from config.h)
    unsigned long airIntervalMs   = AIR_SENSOR_INTERVAL_MS;
    unsigned long waterIntervalMs = WATER_SENSOR_INTERVAL_MS;

    // Latest sensor values
    SensorData current;
};

extern SensorsClass Sensors;

#endif