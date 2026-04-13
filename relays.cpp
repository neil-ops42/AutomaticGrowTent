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
        vegOnHour = s.veg_on_hour;
        vegOffHour = s.veg_off_hour;
        flowerOnHour = s.flower_on_hour;
        flowerOffHour = s.flower_off_hour;
        fanOnTempC = s.fan_on_temp_c;
        fanOffTempC = s.fan_off_temp_c;
        fanMinHysteresisC = s.fan_min_hysteresis_c;
        fanDebounceSec = s.fan_debounce_sec;
        state.autoFan = s.auto_fan;
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
    // If time is unavailable, keep the current light state unchanged.
    // Do NOT default to OFF — that would harm plants during a clock fault.
    struct tm t;
    if (!getLocalTime(&t)) return;

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
    if (!getLocalTime(&t)) return state.light;  // Time unavailable — preserve current state

    int hour = t.tm_hour;

    switch (state.mode)
    {
        case MODE_VEG:
            if (vegOnHour == vegOffHour) return false;
            if (vegOnHour < vegOffHour) return (hour >= vegOnHour && hour < vegOffHour);
            return (hour >= vegOnHour || hour < vegOffHour);

        case MODE_FLOWER:
            if (flowerOnHour == flowerOffHour) return false;
            if (flowerOnHour < flowerOffHour) return (hour >= flowerOnHour && hour < flowerOffHour);
            return (hour >= flowerOnHour || hour < flowerOffHour);

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
        if (stateIn != state.fan) lastFanToggleMs = millis();  // record toggle time
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
    Settings.load(s);  // load existing to preserve non-relay fields
    s.mode = state.mode;
    s.on_hour = customOnHour;
    s.off_hour = customOffHour;
    s.veg_on_hour = vegOnHour;
    s.veg_off_hour = vegOffHour;
    s.flower_on_hour = flowerOnHour;
    s.flower_off_hour = flowerOffHour;
    s.auto_fan = state.autoFan;
    s.fan_on_temp_c = fanOnTempC;
    s.fan_off_temp_c = fanOffTempC;
    s.fan_min_hysteresis_c = fanMinHysteresisC;
    s.fan_debounce_sec = fanDebounceSec;
    Settings.save(s);
}

void RelaysClass::getCustomSchedule(uint8_t& onH, uint8_t& offH)
{
    onH  = customOnHour;
    offH = customOffHour;
}

void RelaysClass::setVegSchedule(uint8_t onH, uint8_t offH)
{
    if (onH > 23 || offH > 23) return;
    vegOnHour = onH;
    vegOffHour = offH;
    saveControlConfig();
}

void RelaysClass::getVegSchedule(uint8_t& onH, uint8_t& offH)
{
    onH = vegOnHour;
    offH = vegOffHour;
}

void RelaysClass::setFlowerSchedule(uint8_t onH, uint8_t offH)
{
    if (onH > 23 || offH > 23) return;
    flowerOnHour = onH;
    flowerOffHour = offH;
    saveControlConfig();
}

void RelaysClass::getFlowerSchedule(uint8_t& onH, uint8_t& offH)
{
    onH = flowerOnHour;
    offH = flowerOffHour;
}

void RelaysClass::setControlConfig(const ControlConfig& in)
{
    customOnHour = (in.customOnHour <= 23) ? in.customOnHour : customOnHour;
    customOffHour = (in.customOffHour <= 23) ? in.customOffHour : customOffHour;
    vegOnHour = (in.vegOnHour <= 23) ? in.vegOnHour : vegOnHour;
    vegOffHour = (in.vegOffHour <= 23) ? in.vegOffHour : vegOffHour;
    flowerOnHour = (in.flowerOnHour <= 23) ? in.flowerOnHour : flowerOnHour;
    flowerOffHour = (in.flowerOffHour <= 23) ? in.flowerOffHour : flowerOffHour;

    state.autoFan = in.autoFan;

    if (!isnan(in.fanOnTempC)) fanOnTempC = in.fanOnTempC;
    if (!isnan(in.fanOffTempC)) fanOffTempC = in.fanOffTempC;
    if (!isnan(in.fanMinHysteresisC) && in.fanMinHysteresisC >= 0.1f && in.fanMinHysteresisC <= 10.0f) fanMinHysteresisC = in.fanMinHysteresisC;
    if (fanOffTempC >= fanOnTempC) fanOffTempC = fanOnTempC - fanMinHysteresisC;
    fanDebounceSec = in.fanDebounceSec;

    state.manualLightOverride = false;
    if (state.autoFan) state.manualFanOverride = false;

    saveControlConfig();
}

ControlConfig RelaysClass::getControlConfig()
{
    ControlConfig out;
    out.customOnHour = customOnHour;
    out.customOffHour = customOffHour;
    out.vegOnHour = vegOnHour;
    out.vegOffHour = vegOffHour;
    out.flowerOnHour = flowerOnHour;
    out.flowerOffHour = flowerOffHour;
    out.fanOnTempC = fanOnTempC;
    out.fanOffTempC = fanOffTempC;
    out.autoFan = state.autoFan;
    out.fanMinHysteresisC = fanMinHysteresisC;
    out.fanDebounceSec = fanDebounceSec;
    return out;
}

void RelaysClass::saveControlConfig()
{
    AppSettings s;
    // Load existing settings first to preserve non-relay fields
    Settings.load(s);
    s.mode = state.mode;
    s.on_hour = customOnHour;
    s.off_hour = customOffHour;
    s.veg_on_hour = vegOnHour;
    s.veg_off_hour = vegOffHour;
    s.flower_on_hour = flowerOnHour;
    s.flower_off_hour = flowerOffHour;
    s.auto_fan = state.autoFan;
    s.fan_on_temp_c = fanOnTempC;
    s.fan_off_temp_c = fanOffTempC;
    s.fan_min_hysteresis_c = fanMinHysteresisC;
    s.fan_debounce_sec = fanDebounceSec;
    Settings.save(s);
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
        bool shouldBeOn = (s.airTemp >= fanOnTempC);
        bool shouldBeOff = (s.airTemp <= fanOffTempC);
        if ((state.fan && shouldBeOff) || (!state.fan && shouldBeOn)) {
            state.manualFanOverride = false;
        } else {
            return;  // Manual override still active — do not touch
        }
    }

    // Debounce: don't allow fan to toggle more than once per configured interval
    unsigned long debounceMs = (unsigned long)fanDebounceSec * 1000UL;
    if (millis() - lastFanToggleMs < debounceMs) return;

    if (!state.fan && s.airTemp >= fanOnTempC) {
        setRelay(2, true, /*fromSchedule=*/ true);
    } else if (state.fan && s.airTemp <= fanOffTempC) {
        setRelay(2, false, /*fromSchedule=*/ true);
    }
}
