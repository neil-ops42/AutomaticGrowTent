#include "relays.h"
#include <time.h>
#include "settings.h"
#include "sensors.h"

RelaysClass Relays;

// ─────────────────────────────────────────────
// Initialize relay hardware + load saved hours
// ─────────────────────────────────────────────
void RelaysClass::begin()
{
    pinMode(PIN_RELAY_1, OUTPUT);
    pinMode(PIN_RELAY_2, OUTPUT);

    // Default ON
    digitalWrite(PIN_RELAY_1, RELAY_ACTIVE_STATE);
    digitalWrite(PIN_RELAY_2, RELAY_ACTIVE_STATE);

    state.light = true;
    state.fan   = true;
    state.manualLightOverride = false;
    state.mode = MODE_CUSTOM;

    // Load from settings file (if present)
    AppSettings s;
    if (Settings.load(s)) {
        customOnHour = s.on_hour;
        customOffHour = s.off_hour;
        state.mode    = s.mode;
    }
}

// ─────────────────────────────────────────────
// Loop: apply schedule unless overridden
// ─────────────────────────────────────────────
void RelaysClass::loop()
{
    updateLightSchedule();
    updateFanAuto();
}

// ─────────────────────────────────────────────
// Light Schedule Logic (only when no manual override)
// Auto-clears manual override at each schedule transition boundary.
// ─────────────────────────────────────────────
void RelaysClass::updateLightSchedule()
{
    bool scheduled = isLightScheduledOn();

    if (state.manualLightOverride) {
        // Auto-clear override when the scheduler would change state,
        // i.e., at the next on/off transition boundary.
        if (scheduled != state.light) {
            state.manualLightOverride = false;
        } else {
            return;  // override still active — do not touch
        }
    }

    if (scheduled != state.light)
        setRelay(1, scheduled, /*fromSchedule*/ true);
}

// ─────────────────────────────────────────────
// Grow‑Mode Lighting Logic
// ─────────────────────────────────────────────
bool RelaysClass::isLightScheduledOn()
{
    struct tm t;
    if (!getLocalTime(&t)) return false;  // Fail safe

    int hour = t.tm_hour;

    switch (state.mode)
    {
        case MODE_VEG:
            // VEG = 18/6 (ON 06:00–23:59, OFF 00:00–05:59)
            // NOTE: Start hour is fixed at 06:00; adjust in code to change.
            return (hour >= 6 && hour < 24);

        case MODE_FLOWER:
            // FLOWER = 12/12 (ON 08:00–19:59, OFF 20:00–07:59)
            // NOTE: Start/end hours are fixed at 08:00/20:00; adjust in code to change.
            return (hour >= 8 && hour < 20);

        case MODE_CUSTOM:
        default:
            // Always OFF if on==off (explicit safety behavior)
            if (customOnHour == customOffHour) return false;
        
            // Same-day window (e.g., 06→18): ON inside the interval
            if (customOnHour < customOffHour) {
                return (hour >= customOnHour && hour < customOffHour);
            }
        
            // Spans midnight (e.g., 18→06 or 06→00): ON late OR early
            return (hour >= customOnHour || hour < customOffHour);
    }
}

// ─────────────────────────────────────────────
// Set Relay State
// fromSchedule=false → manual change: set manual override
// fromSchedule=true  → scheduler applying: do NOT set override
// ─────────────────────────────────────────────
void RelaysClass::setRelay(uint8_t id, bool stateIn, bool fromSchedule)
{
    if (id == 1) {
        state.light = stateIn;
        if (!fromSchedule) state.manualLightOverride = true;
        digitalWrite(PIN_RELAY_1, stateIn ? RELAY_ACTIVE_STATE : RELAY_INACTIVE_STATE);
        return;
    }

    if (id == 2) {
        state.fan = stateIn;
        if (!fromSchedule) state.manualFanOverride = true;
        digitalWrite(PIN_RELAY_2, stateIn ? RELAY_ACTIVE_STATE : RELAY_INACTIVE_STATE);
        return;
    }
}

// ─────────────────────────────────────────────
// Query a relay state
// ─────────────────────────────────────────────
bool RelaysClass::getRelay(uint8_t id)
{
    if (id == 1) return state.light;
    if (id == 2) return state.fan;
    return false;
}

// ─────────────────────────────────────────────
// Report full state struct
// ─────────────────────────────────────────────
RelayState RelaysClass::getState()
{
    return state;
}

// ─────────────────────────────────────────────
// Runtime-editable schedule (persisted)
// ─────────────────────────────────────────────
void RelaysClass::setCustomSchedule(uint8_t onH, uint8_t offH)
{
    if (onH > 23 || offH > 23) return;

    customOnHour  = onH;
    customOffHour = offH;

    // Allow scheduler to take over immediately
    state.manualLightOverride = false;

    // Persist to file with current mode
    AppSettings s;
    s.mode = state.mode;
    s.on_hour = customOnHour;
    s.off_hour = customOffHour;
    Settings.save(s);
}

void RelaysClass::getCustomSchedule(uint8_t& onH, uint8_t& offH)
{
    onH  = customOnHour;
    offH = customOffHour;
}

// ─────────────────────────────────────────────
// Temperature-based automatic fan control
// Uses hysteresis to prevent rapid cycling.
// Auto-clears manual fan override at each transition boundary.
// ─────────────────────────────────────────────
void RelaysClass::updateFanAuto()
{
    if (!state.autoFan) return;

    SensorData s = Sensors.getData();
    if (isnan(s.airTemp)) return;  // No valid reading — leave fan unchanged

    if (state.manualFanOverride) {
        // Auto-clear override when temperature crosses transition threshold
        bool shouldBeOn = (s.airTemp >= FAN_ON_TEMP_C);
        bool shouldBeOff = (s.airTemp <= FAN_OFF_TEMP_C);
        if ((state.fan && shouldBeOff) || (!state.fan && shouldBeOn)) {
            state.manualFanOverride = false;
        } else {
            return;  // Manual override still active — do not touch
        }
    }

    if (!state.fan && s.airTemp >= FAN_ON_TEMP_C) {
        setRelay(2, true, /*fromSchedule=*/ true);
    } else if (state.fan && s.airTemp <= FAN_OFF_TEMP_C) {
        setRelay(2, false, /*fromSchedule=*/ true);
    }
}
