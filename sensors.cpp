#include "sensors.h"

SensorsClass Sensors;

// ─────────────────────────────────────────────
//  Initialize sensors
// ─────────────────────────────────────────────
void SensorsClass::begin()
{
    // Initialize SHT4x
    sht4_ok = sht4.begin(&Wire);
    if (sht4_ok)
    {
        sht4.setPrecision(SHT4X_HIGH_PRECISION);
        sht4.setHeater(SHT4X_NO_HEATER);
    }

    // Initialize DS18B20
    dallas.begin();

    // Force immediate first read
    lastAirRead = 0;
    lastWaterRead = 0;
}

// ─────────────────────────────────────────────
//  Non‑blocking update loop
// ─────────────────────────────────────────────
void SensorsClass::loop()
{
    unsigned long now = millis();

    // Air sensor every 5 sec
    if (now - lastAirRead >= AIR_SENSOR_INTERVAL_MS)
    {
        lastAirRead = now;
        readAir();
    }

    // Water sensor every 5 sec
    if (now - lastWaterRead >= WATER_SENSOR_INTERVAL_MS)
    {
        readWater();
    }
}

// ─────────────────────────────────────────────
//  Read SHT4x (Air Temp + Humidity)
// ─────────────────────────────────────────────
void SensorsClass::readAir()
{
    if (!sht4_ok)
    {
        current.airTemp = NAN;
        current.airHum  = NAN;
        return;
    }

    sensors_event_t hum, temp;
    sht4.getEvent(&hum, &temp);

    current.airTemp = temp.temperature;
    current.airHum  = hum.relative_humidity;
    current.lastUpdate = millis();
}

// ─────────────────────────────────────────────
//  Read DS18B20 (Water Temp)
// ─────────────────────────────────────────────
void SensorsClass::readWater()
{
    static unsigned long requestTime = 0;
    static bool waiting = false;

    unsigned long now = millis();
    
    if (!waiting) {
        // Start request (minimal time)
        dallas.requestTemperatures();
        requestTime = now;
        waiting = true;
        return;  // Don't block
    }
    
    if (now - requestTime < 750) {
        return;  // Wait for temp to be ready
    }
    
    // Now read the result (fast)
    float t = dallas.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C || t == 85.0 || t == -999) {
        current.waterTemp = NAN;
    } else {
        current.waterTemp = t;
    }
    current.lastUpdate = now;
    waiting = false;
    lastWaterRead = now; // interval starts after full read cycle completes
}

// ─────────────────────────────────────────────
//  Get Live SensorData
// ─────────────────────────────────────────────
SensorData SensorsClass::getData()
{
    return current;
}
