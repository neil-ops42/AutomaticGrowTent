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
let ws = new WebSocket("ws://" + location.host + "/ws");

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
<h2>Sensors</h2>
<p>Air Temp: <span id="air">--</span> °C</p>
<p>Humidity: <span id="hum">--</span> %</p>
<p>Water Temp: <span id="water">--</span> °C</p>
<p>Time: <span id="time">--</span></p>
<h3>VPD</h3>
<canvas id="chart_vpd" height="120"></canvas>

<script>
let chartVpd = new Chart(document.getElementById("chart_vpd"), {
  type: "line",
  data: {
    labels: [],
    datasets: [{
      label: "VPD (kPa)",
      data: [],
      borderColor: "purple",
      borderWidth: 2,
      tension: 0.2
    }]
  },
  options: {
    animation: false,
    responsive: true,
    scales: { x: { display: false } }
  }
});

function calcVpd(tempC, rh) {
  if (!isFinite(tempC) || !isFinite(rh)) return null;
  const rhPercent = (rh >= 0 && rh <= 1) ? (rh * 100) : rh;
  if (rhPercent < 0 || rhPercent > 100) return null;
  const svp = 0.6108 * Math.exp((17.27 * tempC) / (tempC + 237.3));
  const vpd = svp * (1 - (rhPercent / 100));
  return Math.round(vpd * 1000) / 1000;
}

setInterval(() => {
  fetch('/data').then(r => r.json()).then(d => {
    document.getElementById("air").innerText = d.air_temp;
    document.getElementById("hum").innerText = d.air_humidity;
    document.getElementById("water").innerText = d.water_temp;
    document.getElementById("time").innerText = d.time;

    const vpd = calcVpd(parseFloat(d.air_temp), parseFloat(d.air_humidity));
    if (vpd !== null) {
      const label = d.time || "";
      const MAX_VPD_POINTS = 300;
      chartVpd.data.labels.push(label);
      chartVpd.data.datasets[0].data.push(vpd);
      if (chartVpd.data.labels.length > MAX_VPD_POINTS) {
        chartVpd.data.labels.shift();
        chartVpd.data.datasets[0].data.shift();
      }
      chartVpd.update();
    }
  });
}, 2000);
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
</p>
<p><b>Fan:</b>
  <button onclick="ws.send('relay2_on')">ON</button>
  <button onclick="ws.send('relay2_off')">OFF</button>
</p>

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
}

// Initialize with current schedule+mode
fetch('/schedule')
  .then(r=>r.json())
  .then(d=>{
    document.getElementById("onTime").value  = String(d.on_hour).padStart(2,"0") + ":00";
    document.getElementById("offTime").value = String(d.off_hour).padStart(2,"0") + ":00";
    if (d.mode) setActiveMode(d.mode);
  });

// Restart device via WebSocket
function restartDevice() {
  if (!confirm('Restart the device? It will take a few seconds to come back online.')) return;
  ws.send('device_restart');
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

<script>
// Chart.js datasets
let airTempData = { labels: [], data: [] };
let airHumData  = { labels: [], data: [] };
let waterData   = { labels: [], data: [] };

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
            scales: { x: { display: false } }
        }
    });
}

let chartAirTemp = makeChart("chart_air_temp", "Air Temp (°C)", "red");
let chartAirHum  = makeChart("chart_air_hum",  "Air Humidity (%)", "blue");
let chartWater   = makeChart("chart_water",    "Water Temp (°C)", "green");

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
};
</script>
)rawliteral";

/*─────────────────────────────────────────────
  HISTORY CONTENT (CSV → Chart.js)
─────────────────────────────────────────────*/
const char HTML_HISTORY[] PROGMEM = R"rawliteral(
<h3>Historical Sensor Charts</h3>

<p>Data loaded from <code>history.csv</code></p>

<canvas id="hist_air_temp" height="120"></canvas>
<canvas id="hist_air_hum"  height="120"></canvas>
<canvas id="hist_water"    height="120"></canvas>

<script>
// Utility: parse CSV into arrays
function parseCSV(text) {
    const lines = text.trim().split("\n");

    // Expect header: time,airTemp,airHum,waterTemp
    const labels = [];
    const airTemp = [];
    const airHum = [];
    const waterTemp = [];

    for (let i = 1; i < lines.length; i++) {
        const parts = lines[i].split(",");
        if (parts.length < 4) continue;

        labels.push(parts[0]);
        airTemp.push(parseFloat(parts[1]));
        airHum.push(parseFloat(parts[2]));
        waterTemp.push(parseFloat(parts[3]));
    }

    return { labels, airTemp, airHum, waterTemp };
}

function makeChart(id, label, color, labels, data) {
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
                pointRadius: 0
            }]
        },
        options: {
            animation: false,
            responsive: true,
            scales: {
                x: { display: false }
            }
        }
    });
}

fetch("/history.csv")
    .then(r => r.text())
    .then(text => {
        const hist = parseCSV(text);

        makeChart("hist_air_temp", "Air Temp (°C)", "red",
                  hist.labels, hist.airTemp);

        makeChart("hist_air_hum", "Air Humidity (%)", "blue",
                  hist.labels, hist.airHum);

        makeChart("hist_water", "Water Temp (°C)", "green",
                  hist.labels, hist.waterTemp);
    });
</script>
)rawliteral";

/*─────────────────────────────────────────────
  OTA (text only; ElegantOTA default at /update)
─────────────────────────────────────────────*/
const char HTML_OTA[] PROGMEM = R"rawliteral(
<h2>OTA Update</h2>
<p>Use <code>/update</code> to access ElegantOTA (default UI).</p>
)rawliteral";
