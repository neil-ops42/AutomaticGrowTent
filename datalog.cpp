#include "datalog.h"
#include "relays.h"

DataLogClass DataLog;

// ─────────────────────────────────────────────
// Initialize LittleFS and ensure CSV is ready
// ─────────────────────────────────────────────
void DataLogClass::begin()
{
    // LittleFS is already mounted by Settings.begin()

    // Ensure the CSV file exists and has a header
    if (!LittleFS.exists(HISTORY_FILE))
    {
        File f = LittleFS.open(HISTORY_FILE, FILE_WRITE);
        if (f)
        {
            f.println("time,airTemp,airHum,waterTemp,vpd,lightOn");
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

    if (now - lastLog >= logIntervalMs)
    {
        lastLog = now;
        writeEntry(Sensors.getData());
    }
}

// ─────────────────────────────────────────────
// Archive current log and start a fresh one
// ─────────────────────────────────────────────
void DataLogClass::rotateLog()
{
    // Remove any previous archive so rename succeeds
    if (LittleFS.exists(HISTORY_OLD_FILE)) {
        LittleFS.remove(HISTORY_OLD_FILE);
    }

    // Archive the current log
    LittleFS.rename(HISTORY_FILE, HISTORY_OLD_FILE);

    // Create a fresh log with just the CSV header
    File hdr = LittleFS.open(HISTORY_FILE, FILE_WRITE);
    if (hdr) {
        hdr.println("time,airTemp,airHum,waterTemp,vpd,lightOn");
        hdr.close();
    }

    Serial.println("Log rotated — old data archived to /history_old.csv");
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

    // Rotate log if it exceeds the size limit to prevent filling LittleFS
    {
        File check = LittleFS.open(HISTORY_FILE, FILE_READ);
        if (check) {
            bool overLimit = check.size() >= maxLogBytes;
            check.close();
            if (overLimit) {
                rotateLog();
            }
        }
    }

    File file = LittleFS.open(HISTORY_FILE, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open history.csv for append");
        return;
    }

    // Use empty fields for NaN values instead of a magic sentinel (-999).
    // parseFloat("") returns NaN in JavaScript, preserving chart behaviour.
    auto fmtVal = [](float v) -> String {
        return isnan(v) ? "" : String(v, 2);
    };

    bool lightOn = Relays.getRelay(1);

    file.printf(
        "%s,%s,%s,%s,%s,%d\n",
        ts,
        fmtVal(data.airTemp).c_str(),
        fmtVal(data.airHum).c_str(),
        fmtVal(data.waterTemp).c_str(),
        fmtVal(data.vpd).c_str(),
        lightOn ? 1 : 0
    );

    file.close();
}
