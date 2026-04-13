#ifndef DATALOG_H
#define DATALOG_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include "config.h"
#include "sensors.h"

class DataLogClass
{
public:
    void begin();
    void loop();

    // Runtime-configurable settings
    void setLogInterval(unsigned long ms)  { logIntervalMs = ms; }
    void setMaxLogBytes(size_t bytes)      { maxLogBytes = bytes; }

private:
    void writeEntry(const SensorData& data);
    void rotateLog();
    unsigned long lastLog = 0;

    // Configurable (default from config.h)
    unsigned long logIntervalMs = LOG_INTERVAL_MS;
    size_t maxLogBytes          = MAX_LOG_BYTES;
};

extern DataLogClass DataLog;

#endif