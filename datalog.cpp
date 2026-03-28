#include "datalog.h"

DataLogClass DataLog;

// ─────────────────────────────────────────────
// Initialize SPIFFS and ensure CSV is ready
// ─────────────────────────────────────────────
void DataLogClass::begin()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS mount FAILED!");
        return;
    }

    // Ensure the CSV file exists and has a header
    if (!SPIFFS.exists(HISTORY_FILE))
    {
        File f = SPIFFS.open(HISTORY_FILE, FILE_WRITE);
        if (f)
        {
            f.println("time,airTemp,airHum,waterTemp");
            f.close();
        }
    }

    lastLog = 0;  // Force first interval evaluation
}

// ─────────────────────────────────────────────
// Periodic logging (non-blocking)
// ─────────────────────────────────────────────
void DataLogClass::loop()
{
    unsigned long now = millis();

    if (now - lastLog >= LOG_INTERVAL_MS)
    {
        lastLog = now;
        writeEntry(Sensors.getData());
    }
}

// ─────────────────────────────────────────────
// Append one CSV record
// ─────────────────────────────────────────────
void DataLogClass::writeEntry(const SensorData& data)
{
    struct tm t;
    if (!getLocalTime(&t))
        return;  // No timestamp? Skip safely.

    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &t);

    File file = SPIFFS.open(HISTORY_FILE, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open history.csv for append");
        return;
    }

    file.printf(
        "%s,%.2f,%.2f,%.2f\n",
        ts,
        isnan(data.airTemp) ? -999.0 : data.airTemp,
        isnan(data.airHum)  ? -999.0 : data.airHum,
        isnan(data.waterTemp) ? -999.0 : data.waterTemp
    );

    file.close();
}