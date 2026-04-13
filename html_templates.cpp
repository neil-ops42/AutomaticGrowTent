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
<title>GrowTentAutomation</title>
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

<h1>GrowTentAutomation</h1>
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
  /* Minimal page-local styles for active mode button */
  .mode-btn { background:#eee; border:1px solid #999; }
  .mode-btn.active { background:#2e7d32; color:#fff; border-color:#1b5e20; }
</style>

<h3>Grow Mode</h3>
<button class="mode-btn" id="btnVeg"     onclick="ws.send('mode_veg');   setActiveMode('veg');">VEG (18/6)</button>
<button class="mode-btn" id="btnFlower"  onclick="ws.send('mode_flower');setActiveMode('flower');">FLOWER (12/12)</button>
<button class="mode-btn" id="btnCustom"  onclick="ws.send('mode_custom');setActiveMode('custom');">CUSTOM</button>
<p>Current mode is highlighted and updates live.</p>

<hr>
<h3>Relays</h3>
<p><b>Light:</b>
  <button onclick="ws.send('relay1_on')">ON</button>
  <button onclick="ws.send('relay1_off')">OFF</button>
  <button onclick="ws.send('schedule_resume')" title="Clear manual override and return to automatic schedule">Resume Schedule</button>
</p>
<p><b>Fan:</b>
  <button onclick="ws.send('relay2_on')">ON</button>
  <button onclick="ws.send('relay2_off')">OFF</button>
</p>
<p id="overrideNotice" style="display:none;color:#c62828;">⚠ Manual override active — schedule paused until next transition or Resume.</p>

<hr>
<h4>Custom Lighting Schedule</h4>

<label>On Time:</label>
<input id="onTime" type="time" value="07:00">

<label>Off Time:</label>
<input id="offTime" type="time" value="01:00">

<button onclick="saveSchedule()">Save</button>

<script>
function saveSchedule() {
    const onStr  = document.getElementById("onTime").value;   // "HH:MM"
    const offStr = document.getElementById("offTime").value;  // "HH:MM"

    if (!onStr || !offStr) {
        alert("Please enter both times.");
        return;
    }

    const onH  = parseInt(onStr.split(":")[0]);
    const offH = parseInt(offStr.split(":")[0]);

    fetch("/schedule", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: "on=" + onH + "&off=" + offH
    }).then(r => r.json())
      .then(j => {
         if (j.ok) alert("Schedule saved!");
         else alert("Error saving schedule.");
       });
}
</script>

<hr>
<h4>VEG Schedule</h4>
<label>On Time:</label>
<input id="vegOnTime" type="time" value="06:00">
<label>Off Time:</label>
<input id="vegOffTime" type="time" value="00:00">

<h4>FLOWER Schedule</h4>
<label>On Time:</label>
<input id="flowerOnTime" type="time" value="08:00">
<label>Off Time:</label>
<input id="flowerOffTime" type="time" value="20:00">

<hr>
<h4>Fan Control</h4>
<label><input id="autoFan" type="checkbox" checked> Auto fan enabled</label><br>
<label>Fan ON temp (°C):</label>
<input id="fanOnTemp" type="number" step="0.1" value="28.0">
<label>Fan OFF temp (°C):</label>
<input id="fanOffTemp" type="number" step="0.1" value="26.0">
<br>
<button onclick="saveControlConfig()">Save All Controls</button>
<script>
function hh(t){ return parseInt((t||"00:00").split(":")[0]); }
async function saveControlConfig() {
  const p = new URLSearchParams();
  p.set("custom_on", hh(document.getElementById("onTime").value));
  p.set("custom_off", hh(document.getElementById("offTime").value));
  p.set("veg_on", hh(document.getElementById("vegOnTime").value));
  p.set("veg_off", hh(document.getElementById("vegOffTime").value));
  p.set("flower_on", hh(document.getElementById("flowerOnTime").value));
  p.set("flower_off", hh(document.getElementById("flowerOffTime").value));
  p.set("auto_fan", document.getElementById("autoFan").checked ? "1" : "0");
  const fanOnVal = document.getElementById("fanOnTemp").value;
  const fanOffVal = document.getElementById("fanOffTemp").value;
  if (fanOnVal !== "") p.set("fan_on_temp", fanOnVal);
  if (fanOffVal !== "") p.set("fan_off_temp", fanOffVal);
  try {
    const res = await fetch("/controls/config", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: p.toString()
    });
    const j = await res.json();
    alert(j.ok ? "Control settings saved!" : "Error saving control settings.");
  } catch (e) {
    alert("Error saving control settings.");
  }
}
</script>
<hr>
<h3>History Data</h3>
<p>
  <a href="/history.csv" download>Download history.csv</a>
</p>

<hr>
<h3>Device Control</h3>
<button style="background:#c62828;color:#fff;" onclick="restartDevice()">Restart Device</button>
<button style="background:#c62828;color:#fff;" onclick="resetSettings()">Reset Settings</button>
<button id="resetHistoryBtn" style="background:#c62828;color:#fff;">Reset History</button>

<script>
// Helper to toggle the green highlight for the active mode button
function setActiveMode(m){
  document.querySelectorAll('.mode-btn').forEach(b=>b.classList.remove('active'));
  if (m === 'veg')      document.getElementById('btnVeg').classList.add('active');
  else if (m === 'flower') document.getElementById('btnFlower').classList.add('active');
  else                  document.getElementById('btnCustom').classList.add('active');
}

// Receive relay/schedule updates via the shared header WebSocket
function onWsMessage(d) {
  if ("mode" in d) setActiveMode(d.mode);
  if ("on_hour" in d) {
    document.getElementById("onTime").value  = String(d.on_hour).padStart(2,"0") + ":00";
    document.getElementById("offTime").value = String(d.off_hour).padStart(2,"0") + ":00";
  }
  if ("veg_on_hour" in d) {
    document.getElementById("vegOnTime").value = String(d.veg_on_hour).padStart(2,"0") + ":00";
    document.getElementById("vegOffTime").value = String(d.veg_off_hour).padStart(2,"0") + ":00";
  }
  if ("flower_on_hour" in d) {
    document.getElementById("flowerOnTime").value = String(d.flower_on_hour).padStart(2,"0") + ":00";
    document.getElementById("flowerOffTime").value = String(d.flower_off_hour).padStart(2,"0") + ":00";
  }
  if ("auto_fan" in d) document.getElementById("autoFan").checked = !!d.auto_fan;
  if ("fan_on_temp_c" in d) document.getElementById("fanOnTemp").value = d.fan_on_temp_c;
  if ("fan_off_temp_c" in d) document.getElementById("fanOffTemp").value = d.fan_off_temp_c;
  if ("manual_override" in d) {
    document.getElementById("overrideNotice").style.display = d.manual_override ? "block" : "none";
  }
}

// Initialize with current schedule+mode
fetch('/settings')
  .then(r=>r.json())
  .then(d=>{
    document.getElementById("onTime").value  = String(d.on_hour).padStart(2,"0") + ":00";
    document.getElementById("offTime").value = String(d.off_hour).padStart(2,"0") + ":00";
    document.getElementById("vegOnTime").value = String(d.veg_on_hour).padStart(2,"0") + ":00";
    document.getElementById("vegOffTime").value = String(d.veg_off_hour).padStart(2,"0") + ":00";
    document.getElementById("flowerOnTime").value = String(d.flower_on_hour).padStart(2,"0") + ":00";
    document.getElementById("flowerOffTime").value = String(d.flower_off_hour).padStart(2,"0") + ":00";
    document.getElementById("autoFan").checked = !!d.auto_fan;
    document.getElementById("fanOnTemp").value = d.fan_on_temp_c;
    document.getElementById("fanOffTemp").value = d.fan_off_temp_c;
    if (d.mode) setActiveMode(d.mode);
  });

// Restart device via WebSocket (requires explicit confirmation token)
function restartDevice() {
  if (!confirm('Restart the device? It will take a few seconds to come back online.')) return;
  ws.send('device_restart_confirm');
  alert('Device is restarting...');
}

// Reset all settings to defaults then restart
function resetSettings() {
  if (!confirm('Reset all settings to defaults? The device will restart.')) return;
  fetch('/settings/reset', { method: 'POST' })
    .then(r => r.json())
    .then(j => {
      if (j.ok) alert('Settings reset. Device is restarting...');
      else alert('Failed to reset settings.');
    })
    .catch(() => alert('Failed to reset settings.'));
}

// Reset history (delete + recreate header)
document.getElementById('resetHistoryBtn').onclick = async () => {
  if (!confirm('Delete history.csv and start fresh? This cannot be undone.')) return;
  try {
    const res = await fetch('/history/reset', { method: 'POST' });
    const ok = res.ok && (await res.json()).ok;
    alert(ok ? 'History cleared.' : 'Failed to clear history.');
  } catch (e) {
    alert('Failed to clear history.');
  }
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
<h3>Historical Sensor Charts</h3>

<p>Data loaded from <code>history.csv</code></p>

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

fetch("/history.csv")
    .then(r => r.text())
    .then(text => {
        const hist = parseCSV(text);

        if (hist.labels.length === 0) {
            const el = document.getElementById("histError");
            el.textContent = "No history data available yet.";
            el.style.display = "";
            return;
        }

        makeChart("hist_air_temp", "Air Temp (°C)", "red",
                  hist.labels, hist.airTemp);

        makeChart("hist_air_hum", "Air Humidity (%)", "blue",
                  hist.labels, hist.airHum);

        makeChart("hist_water", "Water Temp (°C)", "green",
                  hist.labels, hist.waterTemp);

        makeChart("hist_vpd", "VPD (kPa)", "purple",
                  hist.labels, hist.vpd);

        makeChart("hist_light", "Light (1=On, 0=Off)", "orange",
          hist.labels, hist.lightOn, /*stepped=*/true, /*y01=*/true);
    })
    .catch(err => {
        const el = document.getElementById("histError");
        el.textContent = "Failed to load history data: " + err.message;
        el.style.display = "";
    });
</script>
)rawliteral";


