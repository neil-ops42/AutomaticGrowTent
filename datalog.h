#ifndef DATALOG_H
#define DATALOG_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include "config.h"
#include "sensors.h"

class DataLogClass
{
public:
    void begin();
    void loop();

private:
    void writeEntry(const SensorData& data);
    void rotateLog();
    unsigned long lastLog = 0;
};

extern DataLogClass DataLog;

#endif