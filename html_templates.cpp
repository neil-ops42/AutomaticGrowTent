/*─────────────────────────────────────────────
  HTML TEMPLATES — PROGMEM storage
  NOTE: This file is ~17 KB of inline HTML/JS stored in flash (PROGMEM).
  For future maintainability, consider migrating these templates to
  LittleFS-served files (e.g. /www/index.html) so they can be updated
  via OTA without reflashing the firmware.
─────────────────────────────────────────────*/
#include <pgmspace.h>
#include "html_templates.h"

/*─────────────────────────────────────────────
  SHARED HEADER (Theme, Navbar, Status Icons)
─────────────────────────────────────────────*/
const char HTML_HEADER[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Automatic Grow Tent</title>
<style>
  body{font-family:Arial;margin:20px;transition:background .3s,color .3s}
  nav a{margin-right:15px;font-weight:bold;text-decoration:none}
  #statusbar{margin-top:10px}
  .icon{display:inline-block;width:14px;height:14px;border-radius:50%;background:gray;margin-right:5px}
  .icon.on{background:limegreen}
  .icon.off{background:crimson}
  .theme-btn{float:right;cursor:pointer;padding:5px 10px;border:1px solid #888;border-radius:5px}
  .dark{background:#111;color:#eee}
  canvas{max-width:100%;margin-top:20px}
  button{padding:8px 14px;margin:4px;font-size:14px}
  input[type=number]{width:60px}
</style>
</head>
<body>

<div class="theme-btn" onclick="toggleTheme()">Toggle Theme</div>

<h1>Automatic Grow Tent</h1>
<nav>
  <a href="/">Dashboard</a>
  <a href="/controls">Controls</a>
  <a href="/charts">Charts</a>
  <a href="/history">History</a>
  <a href="/update">OTA</a>
</nav>

<div id="statusbar">
  <span class="icon" id="lightIcon"></span> Light
  <span class="icon" id="fanIcon" style="margin-left:15px;"></span> Fan
  Air: <span id="sb_air">--</span>°C
  Hum: <span id="sb_hum">--</span>%
  Water: <span id="sb_water">--</span>°C
</div>

<hr>

<script>
let ws;
// Password for authenticated WebSocket commands.
// Prompts once per page load and caches in memory.
let _wsPass = null;
function getWsPass() {
    if (_wsPass === null) {
        _wsPass = prompt("Enter device password to send commands:") || "";
    }
    return _wsPass;
}
function wsSend(cmd) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({cmd: cmd, pass: getWsPass()}));
    }
}
function connectWS() {
    ws = new WebSocket((location.protocol === "https:" ? "wss://" : "ws://") + location.host + "/ws");

    ws.onmessage = (event) => {
        let j;
        try { j = JSON.parse(event.data); } catch(e) { return; }

        // Sensor data (from periodic broadcast)
        if (j.air_temp !== undefined)
            document.getElementById("sb_air").innerText = j.air_temp;

        if (j.air_humidity !== undefined)
            document.getElementById("sb_hum").innerText = j.air_humidity;

        if (j.water_temp !== undefined)
            document.getElementById("sb_water").innerText = j.water_temp;

        // Relay/mode state (from broadcastRelayState)
        if ("relay1" in j) {
            document.getElementById("lightIcon").className = "icon " + (j.relay1 ? "on" : "off");
            document.getElementById("fanIcon").className   = "icon " + (j.relay2 ? "on" : "off");
        }

        // Restart confirmation from server
        if (j.restarting) { alert('Device is restarting...'); }

        // Dispatch to page-level handler if defined
        if (typeof onWsMessage === "function") onWsMessage(j);
    };

    ws.onclose = () => { setTimeout(connectWS, 3000); };
}
connectWS();
</script>

<script>
// Theme
let theme = localStorage.getItem("theme") || "light";
applyTheme();
function toggleTheme(){ theme = (theme === "light") ? "dark" : "light"; localStorage.setItem("theme", theme); applyTheme(); }
function applyTheme(){ if(theme === "dark") document.body.classList.add("dark"); else document.body.classList.remove("dark"); }
</script>
)rawliteral";

/*─────────────────────────────────────────────
  SHARED FOOTER
─────────────────────────────────────────────*/
const char HTML_FOOTER[] PROGMEM = R"rawliteral(
</body>
</html>
)rawliteral";

/*─────────────────────────────────────────────
  DASHBOARD CONTENT
─────────────────────────────────────────────*/
const char HTML_INDEX[] PROGMEM = R"rawliteral(
<style>
  .dash-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
    gap: 16px;
    margin-top: 20px;
  }
  .dash-card {
    border-radius: 12px;
    padding: 20px 16px;
    text-align: center;
    background: linear-gradient(145deg, #1e2a38, #253446);
    box-shadow: 0 4px 12px rgba(0,0,0,0.4);
    color: #fff;
  }
  .dark .dash-card {
    background: linear-gradient(145deg, #0d1520, #162030);
  }
  .dash-card .card-label {
    font-size: 12px;
    letter-spacing: 1px;
    text-transform: uppercase;
    color: #8eafc9;
    margin-bottom: 10px;
  }
  .dash-card .card-value {
    font-size: 42px;
    font-weight: 700;
    line-height: 1;
    color: #e8f4ff;
  }
  .dash-card .card-unit {
    font-size: 14px;
    color: #8eafc9;
    margin-top: 6px;
  }
  .dash-card.air   .card-value { color: #ff8a65; }
  .dash-card.hum   .card-value { color: #4fc3f7; }
  .dash-card.vpd   .card-value { color: #ce93d8; }
  .dash-card.water .card-value { color: #4db6ac; }
  .light-row {
    display: flex;
    align-items: center;
    gap: 14px;
    margin-top: 24px;
    padding: 16px 20px;
    border-radius: 12px;
    background: linear-gradient(145deg, #1e2a38, #253446);
    box-shadow: 0 4px 12px rgba(0,0,0,0.4);
    color: #fff;
    max-width: 280px;
  }
  .dark .light-row {
    background: linear-gradient(145deg, #0d1520, #162030);
  }
  .light-dot {
    width: 22px;
    height: 22px;
    border-radius: 50%;
    background: #444;
    flex-shrink: 0;
    transition: background 0.3s, box-shadow 0.3s;
  }
  .light-dot.on  { background: #69f0ae; box-shadow: 0 0 10px #69f0ae88; }
  .light-dot.off { background: #ef5350; box-shadow: 0 0 10px #ef535088; }
  .light-label {
    font-size: 13px;
    letter-spacing: 1px;
    text-transform: uppercase;
    color: #8eafc9;
  }
  .light-state {
    font-size: 20px;
    font-weight: 700;
    color: #e8f4ff;
    margin-left: auto;
  }
  .light-state.on  { color: #69f0ae; }
  .light-state.off { color: #ef5350; }
</style>

<h2>Dashboard</h2>

<div class="dash-grid">
  <div class="dash-card air">
    <div class="card-label">Air Temperature</div>
    <div class="card-value" id="dash_air">--</div>
    <div class="card-unit">°C</div>
  </div>
  <div class="dash-card hum">
    <div class="card-label">Humidity</div>
    <div class="card-value" id="dash_hum">--</div>
    <div class="card-unit">%</div>
  </div>
  <div class="dash-card vpd">
    <div class="card-label">VPD</div>
    <div class="card-value" id="dash_vpd">--</div>
    <div class="card-unit">kPa</div>
  </div>
  <div class="dash-card water">
    <div class="card-label">Water Temperature</div>
    <div class="card-value" id="dash_water">--</div>
    <div class="card-unit">°C</div>
  </div>
</div>

<div class="light-row">
  <div class="light-dot" id="dash_light_dot"></div>
  <span class="light-label">Light</span>
  <span class="light-state" id="dash_light_state">--</span>
</div>

<div class="light-row">
  <div class="light-dot" id="dash_fan_dot"></div>
  <span class="light-label">Fan</span>
  <span class="light-state" id="dash_fan_state">--</span>
</div>

<script>
function calcVpd(tempC, rh) {
  if (!isFinite(tempC) || !isFinite(rh)) return null;
  const rhPercent = (rh >= 0 && rh <= 1) ? (rh * 100) : rh;
  if (rhPercent < 0 || rhPercent > 100) return null;
  const svp = 0.6108 * Math.exp((17.27 * tempC) / (tempC + 237.3));
  const vpd = svp * (1 - (rhPercent / 100));
  return Math.round(vpd * 1000) / 1000;
}

function setIndicator(dotId, stateId, on) {
  const dot   = document.getElementById(dotId);
  const state = document.getElementById(stateId);
  dot.className   = "light-dot " + (on ? "on" : "off");
  state.className = "light-state " + (on ? "on" : "off");
  state.innerText = on ? "On" : "Off";
}

function onWsMessage(j) {
  if (j.air_temp !== undefined && j.air_temp !== null)
    document.getElementById("dash_air").innerText = j.air_temp;

  if (j.air_humidity !== undefined && j.air_humidity !== null)
    document.getElementById("dash_hum").innerText = j.air_humidity;

  if (j.water_temp !== undefined && j.water_temp !== null)
    document.getElementById("dash_water").innerText = parseFloat(j.water_temp).toFixed(2);

  if (j.air_temp !== undefined && j.air_humidity !== undefined) {
    const vpd = (j.vpd !== undefined && j.vpd !== null)
      ? j.vpd
      : calcVpd(parseFloat(j.air_temp), parseFloat(j.air_humidity));
    if (vpd !== null)
      document.getElementById("dash_vpd").innerText = parseFloat(vpd).toFixed(2);
  }

  if ("relay1" in j) setIndicator("dash_light_dot", "dash_light_state", j.relay1);
  if ("relay2" in j) setIndicator("dash_fan_dot",   "dash_fan_state",   j.relay2);
}
</script>
)rawliteral";

/*─────────────────────────────────────────────
  CONTROLS CONTENT (with active mode highlight + history tools)
─────────────────────────────────────────────*/
const char HTML_CONTROL[] PROGMEM = R"rawliteral(
<h2>Grow Controls</h2>

<style>
  .ctrl-section{background:linear-gradient(145deg,#1e2a38,#253446);border-radius:12px;padding:18px 20px;margin:16px 0;box-shadow:0 4px 12px rgba(0,0,0,.35);color:#fff}
  .ctrl-section h3{margin:0 0 12px;color:#69f0ae;font-size:16px;text-transform:uppercase;letter-spacing:1px}
  .ctrl-row{display:flex;flex-wrap:wrap;gap:12px;align-items:center;margin:8px 0}
  .ctrl-row label{min-width:140px;font-size:13px;color:#aac4de}
  .ctrl-row input[type=time]{font-size:18px;padding:8px 12px;border-radius:8px;border:2px solid #3a5068;background:#0d1b2a;color:#69f0ae;text-align:center;cursor:pointer;min-width:130px}
  .ctrl-row input[type=time]:focus{border-color:#69f0ae;outline:none;box-shadow:0 0 8px rgba(105,240,174,.3)}
  .ctrl-row input[type=time]::-webkit-calendar-picker-indicator{filter:invert(70%) sepia(50%) saturate(500%) hue-rotate(100deg);cursor:pointer}
  .ctrl-row input[type=number],.ctrl-row input[type=text]{font-size:14px;padding:6px 10px;border-radius:6px;border:2px solid #3a5068;background:#0d1b2a;color:#e0e0e0;width:100px}
  .ctrl-row input[type=number]:focus,.ctrl-row input[type=text]:focus{border-color:#69f0ae;outline:none}
  .ctrl-row select{font-size:14px;padding:6px 10px;border-radius:6px;border:2px solid #3a5068;background:#0d1b2a;color:#e0e0e0}
  .ctrl-row select:focus{border-color:#69f0ae;outline:none}
  .mode-btn{background:#1a2535;border:2px solid #3a5068;color:#aac4de;padding:10px 20px;border-radius:8px;font-size:14px;cursor:pointer;transition:all .2s}
  .mode-btn:hover{border-color:#69f0ae;color:#69f0ae}
  .mode-btn.active{background:#1b5e20;border-color:#69f0ae;color:#fff;font-weight:bold}
  .toggle-switch{position:relative;display:inline-block;width:50px;height:26px;vertical-align:middle}
  .toggle-switch input{opacity:0;width:0;height:0}
  .toggle-slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#3a5068;border-radius:26px;transition:.3s}
  .toggle-slider:before{position:absolute;content:"";height:20px;width:20px;left:3px;bottom:3px;background:#aac4de;border-radius:50%;transition:.3s}
  .toggle-switch input:checked+.toggle-slider{background:#1b5e20}
  .toggle-switch input:checked+.toggle-slider:before{transform:translateX(24px);background:#69f0ae}
  .relay-btn{padding:8px 18px;border:2px solid #3a5068;border-radius:8px;font-size:13px;cursor:pointer;background:#1a2535;color:#aac4de;transition:all .2s}
  .relay-btn:hover{border-color:#69f0ae;color:#69f0ae}
  .relay-btn.on-btn:hover{border-color:#4caf50;color:#4caf50}
  .relay-btn.off-btn:hover{border-color:#ef5350;color:#ef5350}
  .resume-btn{background:#0d47a1;color:#fff;border-color:#1565c0}
  .save-btn{background:linear-gradient(145deg,#1b5e20,#2e7d32);color:#fff;border:none;padding:12px 28px;border-radius:8px;font-size:15px;cursor:pointer;margin-top:12px;transition:all .2s}
  .save-btn:hover{background:linear-gradient(145deg,#2e7d32,#43a047);box-shadow:0 2px 8px rgba(105,240,174,.3)}
  .danger-btn{background:#c62828;color:#fff;border:none;padding:10px 20px;border-radius:8px;font-size:14px;cursor:pointer;margin:4px}
  .danger-btn:hover{background:#e53935}
  .hint{font-size:11px;color:#6b8fad;margin-top:4px}
  #overrideNotice{color:#ef5350;margin:8px 0;display:none}
  .time-pair{display:flex;align-items:center;gap:8px}
  .time-pair .arrow{color:#69f0ae;font-size:20px}
</style>

<!-- GROW MODE -->
<div class="ctrl-section">
  <h3>🌱 Grow Mode</h3>
  <div class="ctrl-row">
    <button class="mode-btn" id="btnVeg" onclick="wsSend('mode_veg');setActiveMode('veg');">VEG (18/6)</button>
    <button class="mode-btn" id="btnFlower" onclick="wsSend('mode_flower');setActiveMode('flower');">FLOWER (12/12)</button>
    <button class="mode-btn" id="btnCustom" onclick="wsSend('mode_custom');setActiveMode('custom');">CUSTOM</button>
  </div>
</div>

<!-- RELAYS -->
<div class="ctrl-section">
  <h3>💡 Relay Control</h3>
  <div class="ctrl-row">
    <label>Light:</label>
    <button class="relay-btn on-btn" onclick="wsSend('relay1_on')">ON</button>
    <button class="relay-btn off-btn" onclick="wsSend('relay1_off')">OFF</button>
    <button class="relay-btn resume-btn" onclick="wsSend('schedule_resume')" title="Clear manual override">Resume Schedule</button>
  </div>
  <div class="ctrl-row">
    <label>Fan:</label>
    <button class="relay-btn on-btn" onclick="wsSend('relay2_on')">ON</button>
    <button class="relay-btn off-btn" onclick="wsSend('relay2_off')">OFF</button>
  </div>
  <p id="overrideNotice">⚠ Manual override active — schedule paused until next transition or Resume.</p>
</div>

<!-- LIGHTING SCHEDULES -->
<div class="ctrl-section">
  <h3>🕐 Lighting Schedules</h3>

  <p style="color:#8eafc9;font-size:13px;margin:0 0 12px">Custom Schedule</p>
  <div class="ctrl-row">
    <label>On Time:</label>
    <div class="time-pair">
      <input id="onTime" type="time" value="07:00">
      <span class="arrow">→</span>
      <input id="offTime" type="time" value="01:00">
      <label style="min-width:auto">Off Time</label>
    </div>
  </div>

  <p style="color:#8eafc9;font-size:13px;margin:12px 0 8px">Veg Schedule</p>
  <div class="ctrl-row">
    <label>On Time:</label>
    <div class="time-pair">
      <input id="vegOnTime" type="time" value="06:00">
      <span class="arrow">→</span>
      <input id="vegOffTime" type="time" value="00:00">
      <label style="min-width:auto">Off Time</label>
    </div>
  </div>

  <p style="color:#8eafc9;font-size:13px;margin:12px 0 8px">Flower Schedule</p>
  <div class="ctrl-row">
    <label>On Time:</label>
    <div class="time-pair">
      <input id="flowerOnTime" type="time" value="08:00">
      <span class="arrow">→</span>
      <input id="flowerOffTime" type="time" value="20:00">
      <label style="min-width:auto">Off Time</label>
    </div>
  </div>
</div>

<!-- FAN CONTROL -->
<div class="ctrl-section">
  <h3>🌀 Fan Control</h3>
  <div class="ctrl-row">
    <label>Auto Fan:</label>
    <label class="toggle-switch"><input id="autoFan" type="checkbox" checked><span class="toggle-slider"></span></label>
  </div>
  <div class="ctrl-row">
    <label>Fan ON temp (°C):</label>
    <input id="fanOnTemp" type="number" step="0.1" value="28.0">
  </div>
  <div class="ctrl-row">
    <label>Fan OFF temp (°C):</label>
    <input id="fanOffTemp" type="number" step="0.1" value="26.0">
  </div>
  <div class="ctrl-row">
    <label>Min hysteresis (°C):</label>
    <input id="fanMinHyst" type="number" step="0.1" min="0.1" max="10" value="0.5">
    <span class="hint">Minimum gap between ON and OFF temps</span>
  </div>
  <div class="ctrl-row">
    <label>Debounce (sec):</label>
    <input id="fanDebounce" type="number" step="1" min="0" max="600" value="30">
    <span class="hint">Min seconds between fan toggles</span>
  </div>
</div>

<!-- TIMEZONE & NTP -->
<div class="ctrl-section">
  <h3>🌍 Timezone &amp; NTP</h3>
  <div class="ctrl-row">
    <label>UTC Offset:</label>
    <select id="gmtOffset">
      <option value="-43200">UTC-12</option><option value="-39600">UTC-11</option>
      <option value="-36000">UTC-10</option><option value="-34200">UTC-9:30</option>
      <option value="-32400">UTC-9</option><option value="-28800">UTC-8 (PST)</option>
      <option value="-25200">UTC-7 (MST)</option><option value="-21600">UTC-6 (CST)</option>
      <option value="-18000" selected>UTC-5 (EST)</option><option value="-14400">UTC-4 (AST)</option>
      <option value="-12600">UTC-3:30</option><option value="-10800">UTC-3</option>
      <option value="-7200">UTC-2</option><option value="-3600">UTC-1</option>
      <option value="0">UTC+0 (GMT)</option><option value="3600">UTC+1 (CET)</option>
      <option value="7200">UTC+2 (EET)</option><option value="10800">UTC+3 (MSK)</option>
      <option value="12600">UTC+3:30</option><option value="14400">UTC+4</option>
      <option value="16200">UTC+4:30</option><option value="18000">UTC+5</option>
      <option value="19800">UTC+5:30 (IST)</option><option value="20700">UTC+5:45</option>
      <option value="21600">UTC+6</option><option value="23400">UTC+6:30</option>
      <option value="25200">UTC+7</option><option value="28800">UTC+8 (CST)</option>
      <option value="31500">UTC+8:45</option><option value="32400">UTC+9 (JST)</option>
      <option value="34200">UTC+9:30</option><option value="36000">UTC+10 (AEST)</option>
      <option value="37800">UTC+10:30</option><option value="39600">UTC+11</option>
      <option value="43200">UTC+12 (NZST)</option><option value="46800">UTC+13</option>
      <option value="50400">UTC+14</option>
    </select>
  </div>
  <div class="ctrl-row">
    <label>DST (Daylight Saving):</label>
    <label class="toggle-switch"><input id="dstEnabled" type="checkbox" checked><span class="toggle-slider"></span></label>
    <span class="hint">Adds +1 hour when enabled</span>
  </div>
  <div class="ctrl-row">
    <label>NTP Server:</label>
    <input id="ntpServer" type="text" style="width:200px" value="ca.pool.ntp.org">
  </div>
</div>

<!-- SENSOR INTERVALS -->
<div class="ctrl-section">
  <h3>📡 Sensor &amp; Logging</h3>
  <div class="ctrl-row">
    <label>Air sensor interval:</label>
    <input id="airInterval" type="number" step="1" min="1" max="3600" value="5"> sec
  </div>
  <div class="ctrl-row">
    <label>Water sensor interval:</label>
    <input id="waterInterval" type="number" step="1" min="1" max="3600" value="5"> sec
  </div>
  <div class="ctrl-row">
    <label>Log interval:</label>
    <input id="logInterval" type="number" step="1" min="10" max="3600" value="60"> sec
  </div>
  <div class="ctrl-row">
    <label>Max log size:</label>
    <input id="maxLogKb" type="number" step="10" min="10" max="4096" value="400"> KB
    <span class="hint">Log rotates when this size is reached</span>
  </div>
</div>

<!-- SAVE ALL -->
<div style="text-align:center;margin:20px 0">
  <button class="save-btn" onclick="saveAllSettings()">💾 Save All Settings</button>
</div>

<script>
function hh(t){ return parseInt((t||"00:00").split(":")[0]); }

async function saveAllSettings() {
  const p = new URLSearchParams();
  // Schedules
  p.set("custom_on", hh(document.getElementById("onTime").value));
  p.set("custom_off", hh(document.getElementById("offTime").value));
  p.set("veg_on", hh(document.getElementById("vegOnTime").value));
  p.set("veg_off", hh(document.getElementById("vegOffTime").value));
  p.set("flower_on", hh(document.getElementById("flowerOnTime").value));
  p.set("flower_off", hh(document.getElementById("flowerOffTime").value));
  // Fan
  p.set("auto_fan", document.getElementById("autoFan").checked ? "1" : "0");
  const fOn = document.getElementById("fanOnTemp").value;
  const fOff = document.getElementById("fanOffTemp").value;
  if (fOn !== "") p.set("fan_on_temp", fOn);
  if (fOff !== "") p.set("fan_off_temp", fOff);
  p.set("fan_min_hysteresis", document.getElementById("fanMinHyst").value);
  p.set("fan_debounce_sec", document.getElementById("fanDebounce").value);
  // Timezone & NTP
  p.set("gmt_offset_sec", document.getElementById("gmtOffset").value);
  p.set("daylight_offset_sec", document.getElementById("dstEnabled").checked ? "3600" : "0");
  p.set("ntp_server", document.getElementById("ntpServer").value);
  // Sensor & Logging
  p.set("air_sensor_interval_sec", document.getElementById("airInterval").value);
  p.set("water_sensor_interval_sec", document.getElementById("waterInterval").value);
  p.set("log_interval_sec", document.getElementById("logInterval").value);
  p.set("max_log_kb", document.getElementById("maxLogKb").value);

  try {
    const res = await fetch("/controls/config", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: p.toString()
    });
    const j = await res.json();
    alert(j.ok ? "All settings saved!" : "Error saving settings.");
  } catch (e) {
    alert("Error saving settings.");
  }
}
</script>

<!-- HISTORY DATA -->
<div class="ctrl-section">
  <h3>📊 History Data</h3>
  <div class="ctrl-row">
    <a href="/history.csv" download style="color:#69f0ae">Download history.csv</a>
    &nbsp;|&nbsp;
    <a href="/history_old.csv" download style="color:#69f0ae">Download history_old.csv</a>
  </div>
</div>

<!-- DEVICE DATA -->
<div class="ctrl-section">
  <h3>🖥 Device Data</h3>
  <style>
    .dev-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:14px;margin:14px 0}
    .dev-card{border-radius:10px;padding:14px 16px;background:linear-gradient(145deg,#152030,#1e2a38);box-shadow:0 2px 8px rgba(0,0,0,.3);color:#fff}
    .dev-card .dc-label{font-size:12px;letter-spacing:1px;text-transform:uppercase;color:#8eafc9;margin-bottom:4px}
    .dev-card .dc-value{font-size:18px;font-weight:700;color:#69f0ae;margin-bottom:6px}
    .dev-card .dc-sub{font-size:12px;color:#aac4de}
    .dev-bar-bg{background:#1a2535;border-radius:4px;height:8px;margin-top:6px;overflow:hidden}
    .dev-bar-fill{height:100%;border-radius:4px;transition:width .4s ease}
    .bar-green{background:#69f0ae}.bar-yellow{background:#ffd54f}.bar-red{background:#ef5350}
  </style>
  <div id="devDataGrid" class="dev-grid">
    <div class="dev-card"><div class="dc-label">Loading…</div></div>
  </div>
  <button class="relay-btn" onclick="loadDeviceData()" style="margin-top:8px">Refresh Device Data</button>
</div>

<script>
const USAGE_WARNING_THRESHOLD=60,USAGE_CRITICAL_THRESHOLD=80;
function fmtBytes(b){if(b>=1048576)return(b/1048576).toFixed(2)+' MB';if(b>=1024)return(b/1024).toFixed(1)+' KB';return b+' B';}
function fmtUptime(sec){const d=Math.floor(sec/86400);sec%=86400;const h=Math.floor(sec/3600);sec%=3600;const m=Math.floor(sec/60);sec%=60;return(d?d+'d ':'')+(h?h+'h ':'')+(m?m+'m ':'')+sec+'s';}
function barClass(pct){return pct<USAGE_WARNING_THRESHOLD?'bar-green':pct<USAGE_CRITICAL_THRESHOLD?'bar-yellow':'bar-red';}
function makeCard(label,valueHtml,subHtml,pct){
  const bar=pct!==null?'<div class="dev-bar-bg"><div class="dev-bar-fill '+barClass(pct)+'" style="width:'+Math.min(pct,100).toFixed(1)+'%"></div></div>':'';
  return '<div class="dev-card"><div class="dc-label">'+label+'</div><div class="dc-value">'+valueHtml+'</div>'+(subHtml?'<div class="dc-sub">'+subHtml+'</div>':'')+bar+'</div>';
}
async function loadDeviceData(){
  const grid=document.getElementById('devDataGrid');
  try{
    const res=await fetch('/api/device');
    if(!res.ok)throw new Error('HTTP '+res.status);
    const d=await res.json();
    const ramPct=parseFloat(d.ram_pct),stPct=parseFloat(d.storage_pct),skPct=parseFloat(d.sketch_pct);
    let html='';
    html+=makeCard('RAM Usage',fmtBytes(d.ram_used)+' / '+fmtBytes(d.ram_total),ramPct.toFixed(1)+'% used &bull; '+fmtBytes(d.ram_free)+' free',ramPct);
    html+=makeCard('Storage Usage',fmtBytes(d.storage_used)+' / '+fmtBytes(d.storage_total),stPct.toFixed(1)+'% used &bull; '+fmtBytes(d.storage_free)+' free',stPct);
    html+=makeCard('Sketch (Firmware)',fmtBytes(d.sketch_used)+' / '+fmtBytes(d.sketch_total),skPct.toFixed(1)+'% used',skPct);
    html+=makeCard('CPU',d.cpu_freq_mhz+' MHz',d.cpu_model+' &bull; '+d.cpu_cores+' core'+(d.cpu_cores>1?'s':''),null);
    html+=makeCard('Flash Size',fmtBytes(d.flash_size),'Total onboard flash',null);
    html+=makeCard('Uptime',fmtUptime(d.uptime_sec),d.uptime_sec+' seconds',null);
    const rssi=d.wifi_rssi;
    const sig=rssi>=-55?'Excellent':rssi>=-67?'Good':rssi>=-70?'Fair':'Weak';
    html+=makeCard('WiFi Signal',rssi+' dBm',sig+' &bull; IP: '+d.wifi_ip,null);
    html+=makeCard('MAC Address','<span style="font-size:13px">'+d.wifi_mac+'</span>','',null);
    html+=makeCard('SDK Version','<span style="font-size:13px">'+d.sdk_version+'</span>','ESP-IDF',null);
    grid.innerHTML=html;
  }catch(e){
    grid.innerHTML='<div class="dev-card"><div class="dc-label">Error</div><div class="dc-value" style="color:#ef5350">Failed to load device data</div></div>';
  }
}
loadDeviceData();
</script>

<!-- DEVICE CONTROL -->
<div class="ctrl-section">
  <h3>⚙ Device Control</h3>
  <div class="ctrl-row">
    <button class="danger-btn" onclick="restartDevice()">Restart Device</button>
    <button class="danger-btn" onclick="resetSettings()">Reset Settings</button>
    <button class="danger-btn" id="resetHistoryBtn">Reset History</button>
  </div>
</div>

<script>
// Mode button highlight
function setActiveMode(m){
  document.querySelectorAll('.mode-btn').forEach(b=>b.classList.remove('active'));
  if(m==='veg')document.getElementById('btnVeg').classList.add('active');
  else if(m==='flower')document.getElementById('btnFlower').classList.add('active');
  else document.getElementById('btnCustom').classList.add('active');
}

// WebSocket live updates
function onWsMessage(d){
  if("mode" in d)setActiveMode(d.mode);
  if("on_hour" in d){
    document.getElementById("onTime").value=String(d.on_hour).padStart(2,"0")+":00";
    document.getElementById("offTime").value=String(d.off_hour).padStart(2,"0")+":00";
  }
  if("veg_on_hour" in d){
    document.getElementById("vegOnTime").value=String(d.veg_on_hour).padStart(2,"0")+":00";
    document.getElementById("vegOffTime").value=String(d.veg_off_hour).padStart(2,"0")+":00";
  }
  if("flower_on_hour" in d){
    document.getElementById("flowerOnTime").value=String(d.flower_on_hour).padStart(2,"0")+":00";
    document.getElementById("flowerOffTime").value=String(d.flower_off_hour).padStart(2,"0")+":00";
  }
  if("auto_fan" in d)document.getElementById("autoFan").checked=!!d.auto_fan;
  if("fan_on_temp_c" in d)document.getElementById("fanOnTemp").value=d.fan_on_temp_c;
  if("fan_off_temp_c" in d)document.getElementById("fanOffTemp").value=d.fan_off_temp_c;
  if("fan_min_hysteresis_c" in d)document.getElementById("fanMinHyst").value=d.fan_min_hysteresis_c;
  if("fan_debounce_sec" in d)document.getElementById("fanDebounce").value=d.fan_debounce_sec;
  if("manual_override" in d){
    document.getElementById("overrideNotice").style.display=d.manual_override?"block":"none";
  }
}

// Load all settings on page load
fetch('/settings').then(r=>r.json()).then(d=>{
  document.getElementById("onTime").value=String(d.on_hour).padStart(2,"0")+":00";
  document.getElementById("offTime").value=String(d.off_hour).padStart(2,"0")+":00";
  document.getElementById("vegOnTime").value=String(d.veg_on_hour).padStart(2,"0")+":00";
  document.getElementById("vegOffTime").value=String(d.veg_off_hour).padStart(2,"0")+":00";
  document.getElementById("flowerOnTime").value=String(d.flower_on_hour).padStart(2,"0")+":00";
  document.getElementById("flowerOffTime").value=String(d.flower_off_hour).padStart(2,"0")+":00";
  document.getElementById("autoFan").checked=!!d.auto_fan;
  document.getElementById("fanOnTemp").value=d.fan_on_temp_c;
  document.getElementById("fanOffTemp").value=d.fan_off_temp_c;
  if(d.fan_min_hysteresis_c!==undefined)document.getElementById("fanMinHyst").value=d.fan_min_hysteresis_c;
  if(d.fan_debounce_sec!==undefined)document.getElementById("fanDebounce").value=d.fan_debounce_sec;
  if(d.gmt_offset_sec!==undefined)document.getElementById("gmtOffset").value=String(d.gmt_offset_sec);
  if(d.daylight_offset_sec!==undefined)document.getElementById("dstEnabled").checked=(d.daylight_offset_sec>0);
  if(d.ntp_server)document.getElementById("ntpServer").value=d.ntp_server;
  if(d.air_sensor_interval_sec)document.getElementById("airInterval").value=d.air_sensor_interval_sec;
  if(d.water_sensor_interval_sec)document.getElementById("waterInterval").value=d.water_sensor_interval_sec;
  if(d.log_interval_sec)document.getElementById("logInterval").value=d.log_interval_sec;
  if(d.max_log_kb)document.getElementById("maxLogKb").value=d.max_log_kb;
  if(d.mode)setActiveMode(d.mode);
});

function restartDevice(){
  if(!confirm('Restart the device? It will take a few seconds to come back online.'))return;
  wsSend('device_restart_confirm');
}

function resetSettings(){
  if(!confirm('Reset all settings to defaults? The device will restart.'))return;
  fetch('/settings/reset',{method:'POST'}).then(r=>r.json()).then(j=>{
    if(j.ok)alert('Settings reset. Device is restarting...');
    else alert('Failed to reset settings.');
  }).catch(()=>alert('Failed to reset settings.'));
}

document.getElementById('resetHistoryBtn').onclick=async()=>{
  if(!confirm('Delete history.csv and start fresh? This cannot be undone.'))return;
  try{
    const res=await fetch('/history/reset',{method:'POST'});
    const ok=res.ok&&(await res.json()).ok;
    alert(ok?'History cleared.':'Failed to clear history.');
  }catch(e){alert('Failed to clear history.');}
};
</script>
)rawliteral";

/*─────────────────────────────────────────────
  CHARTS CONTENT (Live via WebSocket)
─────────────────────────────────────────────*/
const char HTML_CHARTS[] PROGMEM = R"rawliteral(
<h3>Live Sensor Charts</h3>

<canvas id="chart_air_temp" height="120"></canvas>
<canvas id="chart_air_hum"  height="120"></canvas>
<canvas id="chart_water"    height="120"></canvas>
<canvas id="chart_vpd"      height="120"></canvas>

<script>
// Chart.js datasets
let airTempData = { labels: [], data: [] };
let airHumData  = { labels: [], data: [] };
let waterData   = { labels: [], data: [] };

function calcVpd(tempC, rh) {
    if (!isFinite(tempC) || !isFinite(rh)) return null;
    const rhPercent = (rh >= 0 && rh <= 1) ? (rh * 100) : rh;
    if (rhPercent < 0 || rhPercent > 100) return null;
    const svp = 0.6108 * Math.exp((17.27 * tempC) / (tempC + 237.3));
    const vpd = svp * (1 - (rhPercent / 100));
    return Math.round(vpd * 1000) / 1000;
}

function makeChart(id, label, color) {
    return new Chart(document.getElementById(id), {
        type: "line",
        data: {
            labels: [],
            datasets: [{
                label,
                data: [],
                borderColor: color,
                borderWidth: 2,
                tension: 0.2
            }]
        },
        options: {
            animation: false,
            responsive: true,
scales: {
  x: {
    display: true,
    ticks: {
      maxTicksLimit: 8,
      callback: function(value) {
        const label = this.getLabelForValue(value) || "";
        return label.length >= 16 ? label.slice(11, 16) : label; // HH:MM
      }
    }
  }
}
        }
    });
}

let chartAirTemp = makeChart("chart_air_temp", "Air Temp (°C)", "red");
let chartAirHum  = makeChart("chart_air_hum",  "Air Humidity (%)", "blue");
let chartWater   = makeChart("chart_water",    "Water Temp (°C)", "green");
let chartVpd     = makeChart("chart_vpd",      "VPD (kPa)", "purple");

// Receive sensor data via the shared header WebSocket
function onWsMessage(j) {
    if (j.time === undefined) return;
    let t = j.time;

    // Append data
    const MAX_POINTS = 300;
    
    chartAirTemp.data.labels.push(t);
    chartAirTemp.data.datasets[0].data.push(j.air_temp);
    if (chartAirTemp.data.labels.length > MAX_POINTS) {
        chartAirTemp.data.labels.shift();
        chartAirTemp.data.datasets[0].data.shift();
    }
    chartAirTemp.update();

    chartAirHum.data.labels.push(t);
    chartAirHum.data.datasets[0].data.push(j.air_humidity);
    if (chartAirHum.data.labels.length > MAX_POINTS) {
        chartAirHum.data.labels.shift();
        chartAirHum.data.datasets[0].data.shift();
    }    
    chartAirHum.update();

    chartWater.data.labels.push(t);
    chartWater.data.datasets[0].data.push(j.water_temp);
    if (chartWater.data.labels.length > MAX_POINTS) {
        chartWater.data.labels.shift();
        chartWater.data.datasets[0].data.shift();
    }
    chartWater.update();

    const vpd = (j.vpd !== undefined && j.vpd !== null)
        ? j.vpd
        : calcVpd(parseFloat(j.air_temp), parseFloat(j.air_humidity));
    if (vpd !== null) {
        chartVpd.data.labels.push(t);
        chartVpd.data.datasets[0].data.push(vpd);
        if (chartVpd.data.labels.length > MAX_POINTS) {
            chartVpd.data.labels.shift();
            chartVpd.data.datasets[0].data.shift();
        }
        chartVpd.update();
    }
};
</script>
)rawliteral";

/*─────────────────────────────────────────────
  HISTORY CONTENT (CSV → Chart.js)
─────────────────────────────────────────────*/
const char HTML_HISTORY[] PROGMEM = R"rawliteral(
<div style="margin-bottom:12px;">
  <label id="historySourceLabel" for="historySourceSlider"><strong>Data source</strong></label>
  <div style="display:flex;align-items:center;gap:10px;max-width:420px;">
    <span><code>history.csv</code></span>
    <input id="historySourceSlider" type="range" min="0" max="1" step="1" value="0" style="flex:1;" aria-labelledby="historySourceLabel">
    <span><code>history_old.csv</code></span>
  </div>
</div>

<h3>Historical Sensor Charts</h3>

<p>Data loaded from <code id="histSourceText">history.csv</code></p>

<p id="histError" style="color:red;display:none;"></p>
<canvas id="hist_air_temp" height="120"></canvas>
<canvas id="hist_air_hum"  height="120"></canvas>
<canvas id="hist_water"    height="120"></canvas>
<canvas id="hist_vpd"      height="120"></canvas>
<canvas id="hist_light"    height="120"></canvas>

<script>
// Utility: parse CSV into arrays
function calcVpd(tempC, rh) {
    if (tempC === null || rh === null || !isFinite(tempC) || !isFinite(rh)) return null;
    const rhPercent = (rh >= 0 && rh <= 1) ? (rh * 100) : rh;
    if (rhPercent < 0 || rhPercent > 100) return null;
    const svp = 0.6108 * Math.exp((17.27 * tempC) / (tempC + 237.3));
    const vpd = svp * (1 - (rhPercent / 100));
    return Math.round(vpd * 1000) / 1000;
}

function parseCSV(text) {
    const lines = text.trim().split("\n");

    // Expect header: time,airTemp,airHum,waterTemp,vpd,lightOn
    // Also handles legacy format: time,airTemp,airHum,waterTemp,lightOn (5 columns)
    const labels = [];
    const airTemp = [];
    const airHum = [];
    const waterTemp = [];
    const vpd = [];
    const lightOn = [];

    for (let i = 1; i < lines.length; i++) {
        const parts = lines[i].split(",");
        if (parts.length < 4) continue;

        labels.push(parts[0]);
        // Empty fields represent NaN (no sensor reading); parseFloat("") → NaN
        // Values <= -999 are legacy sentinel values and must also be treated as missing.
        const at = parts[1] === "" ? NaN : parseFloat(parts[1]);
        const ah = parts[2] === "" ? NaN : parseFloat(parts[2]);
        const wt = parts[3] === "" ? NaN : parseFloat(parts[3]);
        const atVal = (isNaN(at) || at <= -999) ? null : at;
        const ahVal = (isNaN(ah) || ah <= -999) ? null : ah;
        const wtVal = (isNaN(wt) || wt <= -999) ? null : wt;
        airTemp.push(atVal);
        airHum.push(ahVal);
        waterTemp.push(wtVal);

        // New format (6+ columns) has firmware-computed VPD at index 4, lightOn at index 5.
        // Legacy format (5 columns) has lightOn at index 4; fall back to client-side VPD calc.
        if (parts.length >= 6) {
            const vpdRaw = parts[4] === "" ? NaN : parseFloat(parts[4]);
            vpd.push(isNaN(vpdRaw) ? null : vpdRaw);
            const lo = parts[5] === "" ? NaN : parseInt(parts[5]);
            lightOn.push(isNaN(lo) ? null : (lo ? 1 : 0));
        } else {
            vpd.push(calcVpd(atVal, ahVal));
            const lo = (parts.length >= 5 && parts[4] !== "") ? parseInt(parts[4]) : NaN;
            lightOn.push(isNaN(lo) ? null : (lo ? 1 : 0));
        }
    }

    return { labels, airTemp, airHum, waterTemp, vpd, lightOn };
}

function makeChart(id, label, color, labels, data, stepped=false, y01=false) {
  return new Chart(document.getElementById(id), {
    type: "line",
    data: {
      labels,
      datasets: [{
        label,
        data,
        borderColor: color,
        borderWidth: 2,
        tension: 0.2,
        pointRadius: 0,
        stepped: stepped,
        tension: stepped ? 0 : 0.2
      }]
    },
    options: {
      animation: false,
      responsive: true,
      scales: {
        x: { display: true },
        y: y01 ? { min: 0, max: 1, ticks: { stepSize: 1 } } : {}
      }
    }
  });
}

const histSourceSlider = document.getElementById("historySourceSlider");
const histSourceText = document.getElementById("histSourceText");
const historyCharts = [];
histSourceSlider.setAttribute("aria-valuetext", "history.csv");
histSourceSlider.setAttribute("aria-valuenow", "0");

function selectedHistorySource() {
  return histSourceSlider.value === "1"
    ? { path: "/history_old.csv", name: "history_old.csv" }
    : { path: "/history.csv", name: "history.csv" };
}

function clearHistoryCharts() {
  for (const chart of historyCharts) {
    chart.destroy();
  }
  historyCharts.length = 0;
}

function showHistoryError(message) {
  const el = document.getElementById("histError");
  el.textContent = message;
  el.style.display = "";
}

function hideHistoryError() {
  const el = document.getElementById("histError");
  el.textContent = "";
  el.style.display = "none";
}

async function loadHistoryFromSelectedSource() {
  const src = selectedHistorySource();
  histSourceText.textContent = src.name;
  histSourceSlider.setAttribute("aria-valuetext", src.name);
  histSourceSlider.setAttribute("aria-valuenow", histSourceSlider.value);
  hideHistoryError();
  clearHistoryCharts();

  try {
    const res = await fetch(src.path);
    if (!res.ok) throw new Error(res.status === 404
      ? `${src.name} not found on device.`
      : `HTTP ${res.status}`);

    const text = await res.text();
    const hist = parseCSV(text);

    if (hist.labels.length === 0) {
      showHistoryError(`No history data available in ${src.name}.`);
      return;
    }

    historyCharts.push(makeChart("hist_air_temp", "Air Temp (°C)", "red",
      hist.labels, hist.airTemp));
    historyCharts.push(makeChart("hist_air_hum", "Air Humidity (%)", "blue",
      hist.labels, hist.airHum));
    historyCharts.push(makeChart("hist_water", "Water Temp (°C)", "green",
      hist.labels, hist.waterTemp));
    historyCharts.push(makeChart("hist_vpd", "VPD (kPa)", "purple",
      hist.labels, hist.vpd));
    historyCharts.push(makeChart("hist_light", "Light (1=On, 0=Off)", "orange",
      hist.labels, hist.lightOn, /*stepped=*/true, /*y01=*/true));
  } catch (err) {
    showHistoryError("Failed to load history data: " + err.message);
  }
}

histSourceSlider.addEventListener("change", loadHistoryFromSelectedSource);
loadHistoryFromSelectedSource();
</script>
)rawliteral";


