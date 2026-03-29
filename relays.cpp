#include "relays.h"
#include <time.h>
#include "settings.h"

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
            // Runtime-editable schedule (spans midnight)
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