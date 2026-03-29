#ifndef RELAYS_H
#define RELAYS_H

#include <Arduino.h>
#include "config.h"

// ─────────────────────────────────────────────
// Grow Mode Definitions
// ─────────────────────────────────────────────
enum GrowMode {
    MODE_VEG,       // 18/6
    MODE_FLOWER,    // 12/12
    MODE_CUSTOM     // Uses runtime-editable hours
};

// ─────────────────────────────────────────────
// Relay State Structure
// ─────────────────────────────────────────────
struct RelayState {
    bool light = false;
    bool fan = false;
    bool manualLightOverride = false;  
    bool autoFan = true;               // Whether automatic fan control is enabled
    bool manualFanOverride = false;    // Whether manual fan override is active
    GrowMode mode = MODE_CUSTOM;
};

// ─────────────────────────────────────────────
// Relay Control Class
// ─────────────────────────────────────────────
class RelaysClass
{
public:
    void begin();
    void loop();

    // NOTE: fromSchedule=false means "manual change" and sets manual override.
    void setRelay(uint8_t id, bool state, bool fromSchedule = false);
    bool getRelay(uint8_t id);

    RelayState getState();

    // Must be public so WebSocket handler can change mode
    RelayState state;

    // New: runtime-editable schedule (used when MODE_CUSTOM)
    void setCustomSchedule(uint8_t onH, uint8_t offH);
    void getCustomSchedule(uint8_t& onH, uint8_t& offH);

    // Clear manual override and let the scheduler resume immediately
    void resumeSchedule() { state.manualLightOverride = false; }

private:
    void updateLightSchedule();
    bool isLightScheduledOn();
    void updateFanAuto();  // temperature-based fan control

    // Default to config values; editable at runtime
    uint8_t customOnHour  = LIGHT_ON_HOUR;
    uint8_t customOffHour = LIGHT_OFF_HOUR;
};

extern RelaysClass Relays;

#endif