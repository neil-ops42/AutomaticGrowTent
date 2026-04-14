# Grow Tent Automation (ESP32)

ESP32 firmware for monitoring and controlling a small indoor grow tent: reads environmental sensors, controls relays for grow lights and circulation fan, logs data to LittleFS, and serves a real-time web dashboard with WebSocket updates — all on a single microcontroller.

---

## Highlights

- **Sensors**
  - Air temperature & humidity: **SHT4x** (I²C)
  - Water / reservoir temperature: **DS18B20** (OneWire)
- **Relay control**
  - Relay 1: **Grow light** — scheduled (Veg 18/6, Flower 12/12, or Custom)
  - Relay 2: **Circulation fan** — temperature-based auto-control with manual override
- **Scheduling**
  - Built-in grow modes: **Veg (18/6)**, **Flower (12/12)**, **Custom**
  - Custom schedule editable at runtime via web UI; persisted to flash
- **UI / observability**
  - Responsive web dashboard (live readings, charts, controls, history)
  - **WebSocket** live updates to all connected clients
  - **SSD1306 128×64 OLED** status display (readings, relay states)
- **Reliability & maintenance**
  - **LittleFS** CSV logging (`/history.csv`) with automatic rotation
  - **NTP** time sync with configurable UTC offset and DST
  - **OTA** firmware updates via **ElegantOTA** (no USB cable after first flash)
  - Hardware watchdog

---

## Repo Contents

| File / Module | Purpose |
|---|---|
| `GrowTentAutomation.ino` | Main sketch — `setup()`, `loop()`, Wi-Fi, NTP |
| `config.h` | Pin definitions, timing constants, defaults |
| `secrets.h.example` | Template — copy to `secrets.h` and fill in credentials |
| `sensors.h / .cpp` | SHT4x + DS18B20 reading logic |
| `relays.h / .cpp` | Relay control, grow-mode scheduling, fan auto-control |
| `settings.h / .cpp` | Persistent settings (LittleFS load/save) |
| `datalog.h / .cpp` | CSV data logger (LittleFS) |
| `ui_oled.h / .cpp` | OLED display rendering |
| `webserver.h / .cpp` | Async HTTP server + WebSocket handler |
| `html_templates.h / .cpp` | HTML pages stored in PROGMEM |
| `platformio.ini` | PlatformIO project config and library dependencies |

---

## Hardware

### Required parts

- **ESP32** development board (e.g. ESP32-DevKitC, NodeMCU-32S)
- **SHT4x** temperature / humidity sensor (I²C)
- **DS18B20** waterproof temperature probe (OneWire) + **4.7 kΩ** pull-up resistor to 3.3 V
- **2-channel relay module** (active-HIGH per default config)
- **SSD1306 128×64 OLED** display (I²C, address `0x3C`)
- Appropriate power supply for the ESP32 and relay loads

### Default pin map

| Function | GPIO |
|---|---|
| I²C SDA | 21 |
| I²C SCL | 22 |
| DS18B20 OneWire | 14 |
| Relay 1 — Light | 27 |
| Relay 2 — Fan | 26 |

> To use different pins, update the assignments in `config.h`.

---

## Wiring Quick Map

```
ESP32 GPIO 21 (SDA) ──── SHT4x SDA  ──── SSD1306 SDA
ESP32 GPIO 22 (SCL) ──── SHT4x SCL  ──── SSD1306 SCL
ESP32 GPIO 14       ──── DS18B20 Data (4.7 kΩ pull-up to 3.3 V)
ESP32 GPIO 27       ──── Relay module IN1  (Light)
ESP32 GPIO 26       ──── Relay module IN2  (Fan)
3.3 V / GND         ──── Sensor VCC/GND, relay VCC/GND
```

---

## Build & Flash

### Option A: PlatformIO (recommended)

1. Install [VS Code](https://code.visualstudio.com/) + [PlatformIO extension](https://platformio.org/)
2. Clone and open the project:

   ```bash
   git clone https://github.com/neiltuttle-ops/GrowTentAutomation.git
   cd GrowTentAutomation
   ```

3. Copy the secrets template and fill in your credentials:

   ```bash
   cp secrets.h.example secrets.h
   ```

   ```cpp
   const char* WIFI_SSID         = "YourNetworkName";
   const char* WIFI_PASSWORD     = "YourPassword";
   const char* WEB_AUTH_USERNAME = "admin";
   const char* WEB_AUTH_PASSWORD = "changeme";  // change before deploying
   ```

4. Run **PIO: Build** then **PIO: Upload** from the VS Code command palette (or use the toolbar buttons). All library dependencies are fetched automatically from `platformio.ini`.

### Option B: Arduino IDE

1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Install ESP32 board support via **Boards Manager**
3. Install the following libraries via **Library Manager**:
   - `Adafruit SHT4x`
   - `Adafruit SSD1306` + `Adafruit GFX`
   - `OneWire`
   - `DallasTemperature`
   - `ESPAsyncWebServer` + `AsyncTCP`
   - `ElegantOTA`
   - `ArduinoJson`
4. Copy `secrets.h.example` to `secrets.h` and fill in your credentials (same values as above)
5. Open `GrowTentAutomation.ino`, select your ESP32 board and port, then click **Upload**
6. Open **Serial Monitor** at **115200 baud** to find the assigned IP address

---

## First Run

1. Power the ESP32 (USB or external supply)
2. Check the Serial Monitor for the assigned IP, or find it in your router's DHCP leases
3. Open the dashboard in a browser:

   ```
   http://<esp32-ip>/
   ```

---

## Web UI Endpoints

| Page / Feature | Path | Notes |
|---|---|---|
| Dashboard | `/` | Live sensor readings and system status |
| Controls | `/controls` | Toggle relays, set grow mode, edit custom schedule |
| Charts | `/charts` | Graphs built from logged sensor data |
| History | `/history` | Raw CSV view / download |
| OTA Update | `/update` | Upload firmware `.bin` over Wi-Fi (auth required) |
| Settings Backup | `/settings/backup` | `GET` — download current settings file (auth required) |
| Settings Restore | `/settings/restore` | `POST` — upload a settings file to restore (auth required) |
| Settings Reset | `/settings/reset` | `POST` — reset to defaults and reboot (auth required) |
| Archived log | `/history_old.csv` | Previous log file after rotation |
| Sensor snapshot | `/data` | `GET` — current sensor readings as JSON |
| All settings | `/settings` | `GET` — current settings as JSON |
| Device metrics | `/api/device` | `GET` — RAM, storage, CPU, Wi-Fi info as JSON |
| History reset | `/history/reset` | `POST` — clear the sensor log (auth required) |
| Light ON | `/relay1/on` | `POST` — turn grow light on (auth required) |
| Light OFF | `/relay1/off` | `POST` — turn grow light off (auth required) |
| Fan ON | `/relay2/on` | `POST` — turn circulation fan on (auth required) |
| Fan OFF | `/relay2/off` | `POST` — turn circulation fan off (auth required) |
| Custom schedule | `/schedule` | `POST` `on=HH&off=HH` — set custom light schedule (auth required) |
| Full config | `/controls/config` | `POST` — update schedules, fan thresholds, timezone, intervals (auth required) |

Real-time relay and sensor state is pushed to all connected clients over a **WebSocket** at `/ws`.

### Security Note

The following endpoints are protected by **HTTP Basic Authentication**. Credentials are set in `secrets.h`:

- `/update`
- `/settings/reset`
- `/settings/restore`
- `/settings/backup`
- `/history/reset`
- `/relay1/on` · `/relay1/off` · `/relay2/on` · `/relay2/off`
- `/schedule`
- `/controls/config`

Use a strong, unique password in `secrets.h` — the placeholder `changeme` in `secrets.h.example` is intentionally weak.

---

## Configuration Knobs

Key constants in `config.h`:

| Constant | Default | Description |
|---|---:|---|
| `GMT_OFFSET_SEC` | `-18000` | UTC offset in seconds (EST = UTC−5) |
| `DAYLIGHT_OFFSET_SEC` | `3600` | DST adjustment in seconds |
| `LIGHT_ON_HOUR` | `7` | Default light-on hour (custom mode) |
| `LIGHT_OFF_HOUR` | `1` | Default light-off hour (custom mode) |
| `LOG_INTERVAL_MS` | `60000` | Sensor log interval (60 s) |
| `AIR_SENSOR_INTERVAL_MS` | `5000` | SHT4x polling interval (5 s) |
| `WATER_SENSOR_INTERVAL_MS` | `5000` | DS18B20 polling interval (5 s) |
| `MAX_LOG_BYTES` | `409600` | Log rotation threshold (400 KB) |
| `FAN_ON_TEMP_C` | `28.0` | Air temp threshold to turn fan ON (auto mode) |
| `FAN_OFF_TEMP_C` | `26.0` | Air temp threshold to turn fan OFF (auto mode) |
| `FAN_MIN_HYSTERESIS_C` | `0.5` | Minimum gap enforced between `FAN_ON_TEMP_C` and `FAN_OFF_TEMP_C` |
| `FAN_DEBOUNCE_MS` | `30000` | Minimum time (ms) the fan must stay in its current state before switching |

Most control defaults are now editable from the **Controls** page at runtime and persisted to flash: custom/veg/flower schedule hours, auto-fan toggle, fan ON/OFF temperature thresholds, minimum hysteresis, and debounce period.

### Fan control

The circulation fan (Relay 2) supports two modes selectable from the Controls page:

- **Auto** — turns the fan on when air temperature exceeds `FAN_ON_TEMP_C` and off again once it drops below `FAN_OFF_TEMP_C`. A minimum hysteresis gap (`FAN_MIN_HYSTERESIS_C`) is enforced between the two thresholds, and a debounce timer (`FAN_DEBOUNCE_MS`) prevents rapid cycling by requiring the fan to remain in its current state for at least that long before switching. The manual override is automatically cleared when auto-control activates due to a temperature transition.
- **Manual** — toggle the relay directly from the Controls page; the manual override persists until auto-control clears it at the next temperature transition.

---

## Data Logging

Sensor readings are appended to `/history.csv` on LittleFS every `LOG_INTERVAL_MS` (default 60 s). When the file exceeds `MAX_LOG_BYTES` (default 400 KB), it is rotated:

1. The current `/history.csv` is copied to `/history_old.csv` (overwriting any previous archive)
2. A new `/history.csv` is started

Both files are accessible from the web UI.

---

## OTA Updates

After the initial USB flash, all subsequent firmware updates can be done wirelessly:

1. Navigate to `http://<esp32-ip>/update`
2. Upload the compiled `.bin` file
3. The device reboots automatically into the new firmware

Powered by [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA).

---

## License

This project is licensed under the **GNU General Public License v2.0** — see the [LICENSE](LICENSE) file for details.
