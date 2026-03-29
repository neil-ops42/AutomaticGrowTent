# 🌱 Grow Tent Automation

An ESP32-based automation system for monitoring and controlling a small indoor grow tent. The firmware reads environmental sensors, drives relay-controlled lights and fans on configurable schedules, logs data to onboard flash, and serves a real-time web dashboard — all from a single microcontroller.

---

## Features

| Category | Details |
|---|---|
| **Environmental Sensing** | Air temperature & humidity via **SHT4x** (I²C); water / reservoir temperature via **DS18B20** (OneWire) |
| **Relay Control** | Two relay channels — **grow light** and **exhaust fan** — with independent on/off control |
| **Grow-Mode Scheduling** | Pre-set **Veg (18/6)**, **Flower (12/12)**, and fully **Custom** light schedules editable at runtime |
| **Manual Override** | Toggle any relay on or off from the web UI; manual state persists until the next scheduled change |
| **Web Dashboard** | Responsive async web server with pages for live sensor readings, relay control, historical charts, and data history |
| **WebSocket Updates** | Real-time push of relay & sensor state to all connected browsers |
| **OLED Display** | 128×64 SSD1306 OLED shows current readings, relay states, and a Wi-Fi signal-strength icon |
| **Data Logging** | Sensor readings written to **LittleFS** (`/history.csv`) every 60 seconds; when the file exceeds 100 KB it is archived to `/history_old.csv` and a fresh log is started |
| **OTA Updates** | Firmware updates over Wi-Fi via **ElegantOTA** — no USB cable required after initial flash |
| **Persistent Settings** | Grow mode and custom schedule saved to LittleFS and restored on reboot; downloadable / uploadable via web endpoints |
| **NTP Time Sync** | Automatic clock synchronisation from `ca.pool.ntp.org` with configurable UTC offset and DST |
| **Watchdog** | Hardware watchdog timer keeps the system responsive; resets if a module stalls |

---

## Hardware Requirements

- **ESP32** development board (e.g. ESP32-DevKitC, NodeMCU-32S)
- **SHT4x** temperature / humidity sensor (I²C — SDA pin 21, SCL pin 22)
- **DS18B20** waterproof temperature probe (OneWire — pin 14)
- **2-channel relay module** (light on pin 27, fan on pin 26; active-HIGH logic)
- **SSD1306 128×64 OLED** display (I²C — address `0x3C`)
- Appropriate power supply for the ESP32 and relay loads

---

## Pin Map

| Function | GPIO |
|---|---|
| SDA (I²C) | 21 |
| SCL (I²C) | 22 |
| DS18B20 OneWire | 14 |
| Relay 1 — Light | 27 |
| Relay 2 — Fan | 26 |

---

## Project Structure

```
growtentautomation/
├── GrowTentAutomation.ino   # Main sketch — setup(), loop(), WiFi, NTP
├── config.h                 # Pin definitions, timing constants, defaults
├── secrets.h.example        # Template for WiFi credentials + web auth (copy → secrets.h)
├── sensors.h / .cpp         # SHT4x + DS18B20 reading logic
├── relays.h / .cpp          # Relay control, grow-mode scheduling
├── settings.h / .cpp        # Persistent settings (LittleFS load/save)
├── datalog.h / .cpp         # CSV data logger (LittleFS)
├── ui_oled.h / .cpp         # OLED display rendering + WiFi icon
├── webserver.h / .cpp       # Async HTTP server + WebSocket handler
├── html_templates.h / .cpp  # HTML pages stored in PROGMEM
├── platformio.ini           # PlatformIO project config
├── LICENSE                  # GNU GPL v2
└── .gitignore
```

---

## Getting Started

### Prerequisites

- [Arduino IDE](https://www.arduino.cc/en/software) **2.x** (or PlatformIO — see `platformio.ini`)
- **ESP32 board package** installed via Boards Manager
- The following Arduino libraries (install via Library Manager):
  - `Adafruit SHT4x`
  - `Adafruit SSD1306` (+ `Adafruit GFX`)
  - `OneWire`
  - `DallasTemperature`
  - `ESPAsyncWebServer` + `AsyncTCP`
  - `ElegantOTA`
  - `ArduinoJson`

### Installation

1. **Clone the repository**

   ```bash
git clone https://github.com/neiltuttle-ops/growtentautomation.git
cd growtentautomation
```

2. **Create your secrets file**

   ```bash
cp secrets.h.example secrets.h
```

   Open `secrets.h` and replace the placeholders with your Wi-Fi credentials and a web auth password:

   ```cpp
const char* WIFI_SSID         = "YourNetworkName";
const char* WIFI_PASSWORD     = "YourPassword";
const char* WEB_AUTH_USERNAME = "admin";       // used for /update, /settings/reset, /settings/restore
const char* WEB_AUTH_PASSWORD = "changeme";    // ← change before deploying!
```

3. **Review `config.h`** — adjust timezone offsets, pin assignments, or default light schedule if needed.

4. **Open `GrowTentAutomation.ino`** in the Arduino IDE, select your ESP32 board and port, then **Upload**.

5. **Open Serial Monitor** at 115200 baud to see the assigned IP address, then navigate to it in a browser.

---

## Wiring

> **TODO:** A full wiring diagram (Fritzing / KiCad) will be added here in a future update.

Below is a quick text summary of the key connections:

```
ESP32 GPIO 21 (SDA) ──── SHT4x SDA  ──── SSD1306 SDA
ESP32 GPIO 22 (SCL) ──── SHT4x SCL  ──── SSD1306 SCL
ESP32 GPIO 14       ──── DS18B20 Data (4.7 kΩ pull-up to 3.3 V)
ESP32 GPIO 27       ──── Relay module IN1  (Light)
ESP32 GPIO 26       ──── Relay module IN2  (Fan)
3.3 V / GND         ──── Sensor VCC/GND, relay VCC/GND
```

---

## Web Interface

Once running, the ESP32 hosts a web server on port **80** with the following pages:

| Page | Path | Description |
|---|---|---|
| Dashboard | `/` | Live sensor readings and system status |
| Control | `/controls` | Toggle relays, switch grow modes, set custom schedule |
| Charts | `/charts` | Graphical view of logged sensor data |
| History | `/history` | Raw CSV data download / view |
| OTA Update | `/update` | Upload new firmware binaries over Wi-Fi |
| Settings Backup | `/settings/backup` | `GET` — download `settings.txt` as `settings_backup.txt` (requires auth) |
| Settings Restore | `/settings/restore` | `POST` — upload a settings file to overwrite `settings.txt` and reload (requires auth) |
| Archived Log | `/history_old.csv` | `GET` — download the previous log archived during the last rotation |

> **Security note:** The `/update`, `/settings/reset`, `/settings/restore`, and `/settings/backup` endpoints are protected by HTTP Basic Authentication. Credentials are configured in `secrets.h` (see `secrets.h.example`).

Real-time relay and sensor state is pushed to all connected clients over a **WebSocket** at `/ws`.

---

## Configuration

Key constants in `config.h`:

| Constant | Default | Description |
|---|---|---|
| `GMT_OFFSET_SEC` | `-18000` | UTC offset in seconds (EST = UTC−5) |
| `DAYLIGHT_OFFSET_SEC` | `3600` | DST adjustment in seconds |
| `LIGHT_ON_HOUR` | `7` | Default light-on hour (custom mode) |
| `LIGHT_OFF_HOUR` | `1` | Default light-off hour (custom mode) |
| `LOG_INTERVAL_MS` | `60000` | Sensor log interval (1 min) |
| `AIR_SENSOR_INTERVAL_MS` | `5000` | SHT4x polling interval |
| `WATER_SENSOR_INTERVAL_MS` | `5000` | DS18B20 polling interval |
| `MAX_LOG_BYTES` | `102400` | Max `/history.csv` size before log rotation (100 KB) |

> **Fan control:** The fan relay (Relay 2) is currently manual-only — it does not follow a schedule. Toggle it from the Controls page via WebSocket. Automatic fan scheduling is a planned future enhancement.

---

## PlatformIO

A `platformio.ini` is included for use with [PlatformIO](https://platformio.org/). Open the project folder in VS Code with the PlatformIO extension and it will pick up the configuration automatically. All library dependencies are declared in the ini file; run **PIO: Build** to fetch and compile.

---

## OTA (Over-the-Air) Updates

After the initial USB flash, subsequent firmware updates can be uploaded wirelessly:

1. Navigate to `http://<esp32-ip>/update`
2. Select the compiled `.bin` file
3. Upload — the ESP32 will reboot with the new firmware

Powered by [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA).

---

## License

This project is licensed under the **GNU General Public License v2.0** — see the [LICENSE](LICENSE) file for details.