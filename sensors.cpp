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

    // Air sensor every configured interval
    if (now - lastAirRead >= airIntervalMs)
    {
        lastAirRead = now;
        readAir();
    }

    // Water sensor every configured interval
    if (now - lastWaterRead >= waterIntervalMs)
    {
        readWater();
    }
}

// ─────────────────────────────────────────────
//  Compute Vapor Pressure Deficit (kPa)
// ─────────────────────────────────────────────
float SensorsClass::calcVPD(float tempC, float rh)
{
    if (isnan(tempC) || isnan(rh)) return NAN;
    // Magnus-Tetens approximation: SVP in kPa, then VPD = SVP * (1 - RH/100)
    float svp = 0.6108f * expf((17.27f * tempC) / (tempC + 237.3f));
    return svp * (1.0f - (rh / 100.0f));
}

// ─────────────────────────────────────────────
//  Read SHT4x (Air Temp + Humidity)
// ─────────────────────────────────────────────
void SensorsClass::readAir()
{
    if (!sht4_ok)
    {
        // Sensor failed previously — attempt reinitialization before giving up
        sht4_ok = sht4.begin(&Wire);
        if (sht4_ok)
        {
            sht4.setPrecision(SHT4X_HIGH_PRECISION);
            sht4.setHeater(SHT4X_NO_HEATER);
            Serial.println("SHT4x reinitialized successfully");
        }
        else
        {
            Serial.println("SHT4x reinitialization failed");
            current.airTemp = NAN;
            current.airHum  = NAN;
            current.vpd     = NAN;
            return;
        }
    }

    sensors_event_t hum, temp;
    if (!sht4.getEvent(&hum, &temp))
    {
        // I2C read failed — store NaN and allow reinitialization on next interval
        sht4_ok = false;
        current.airTemp = NAN;
        current.airHum  = NAN;
        current.vpd     = NAN;
        Serial.println("SHT4x read failed — will attempt reinitialization on next interval");
        return;
    }

    current.airTemp = roundf(temp.temperature * 100.0f) / 100.0f;
    current.airHum  = roundf(hum.relative_humidity * 100.0f) / 100.0f;
    current.vpd     = calcVPD(current.airTemp, current.airHum);
    current.lastUpdate = millis();
    if (!isnan(current.airTemp)) current.lastValidAirRead = current.lastUpdate;
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
        current.lastValidWaterRead = now;
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
