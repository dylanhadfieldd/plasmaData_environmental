# ESP32 Environmental Monitoring System for Cold Atmospheric Plasma Undergrad Research

This project is focused on monitoring and data logging for:

- Temperature (SHTC3)
- Humidity (SHTC3)
- Distance (VL6180X)

This repo is used in combination with [plasmaData_analysis](https://github.com/dylanhadfieldd/plasmaData_analysis.git) for downstream analysis and reproducibility.

## Project Layout

```text
.
|-- include/
|   |-- AppTypes.h
|   |-- Config.h
|   |-- SensorManager.h
|   |-- TrendBuffer.h
|   `-- UiRenderer.h
|-- scripts/
|   `-- start-ui-tester.ps1
|-- src/
|   |-- main.cpp
|   |-- SensorManager.cpp
|   `-- UiRenderer.cpp
|-- ui-tester/
|   |-- app.js
|   |-- assets/
|   |   `-- capture-healing-logo.png
|   |-- index.html
|   `-- styles.css
|-- platformio.ini
`-- README.md
```

## Pinout (ESP32)

### Primary Wiring Table

<table>
  <thead>
    <tr>
      <th><span style="color:#1565c0;">Function</span></th>
      <th><span style="color:#c62828;">Device Pin</span></th>
      <th><span style="color:#6a1b9a;">ESP32 Pin</span></th>
      <th><span style="color:#000000;">Notes</span></th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><span style="color:#1565c0;">I2C SDA</span></td>
      <td><span style="color:#c62828;">SHTC3 SDA + VL6180X SDA</span></td>
      <td><span style="color:#6a1b9a;">GPIO21</span></td>
      <td><span style="color:#000000;">Shared I2C bus</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">I2C SCL</span></td>
      <td><span style="color:#c62828;">SHTC3 SCL + VL6180X SCL</span></td>
      <td><span style="color:#6a1b9a;">GPIO22</span></td>
      <td><span style="color:#000000;">Shared I2C bus</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">VL6180X XSHUT</span></td>
      <td><span style="color:#c62828;">XSHUT</span></td>
      <td><span style="color:#6a1b9a;">GPIO27</span></td>
      <td><span style="color:#000000;">Optional but supported in firmware</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">TFT SPI SCK</span></td>
      <td><span style="color:#c62828;">SCK</span></td>
      <td><span style="color:#6a1b9a;">GPIO18</span></td>
      <td><span style="color:#000000;">SPI clock</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">TFT SPI MISO</span></td>
      <td><span style="color:#c62828;">MISO</span></td>
      <td><span style="color:#6a1b9a;">GPIO19</span></td>
      <td><span style="color:#000000;">Optional on some TFTs, kept configured</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">TFT SPI MOSI</span></td>
      <td><span style="color:#c62828;">MOSI</span></td>
      <td><span style="color:#6a1b9a;">GPIO23</span></td>
      <td><span style="color:#000000;">SPI data</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">TFT CS</span></td>
      <td><span style="color:#c62828;">CS</span></td>
      <td><span style="color:#6a1b9a;">GPIO14</span></td>
      <td><span style="color:#000000;">TFT chip select</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">TFT DC</span></td>
      <td><span style="color:#c62828;">DC/RS</span></td>
      <td><span style="color:#6a1b9a;">GPIO26</span></td>
      <td><span style="color:#000000;">TFT data/command</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">TFT RST</span></td>
      <td><span style="color:#c62828;">RST</span></td>
      <td><span style="color:#6a1b9a;">GPIO4</span></td>
      <td><span style="color:#000000;">TFT reset</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">TFT Backlight</span></td>
      <td><span style="color:#c62828;">BL/LED</span></td>
      <td><span style="color:#6a1b9a;">GPIO15</span></td>
      <td><span style="color:#000000;">Set HIGH in firmware</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">3.3V Power</span></td>
      <td><span style="color:#c62828;">VCC</span></td>
      <td><span style="color:#6a1b9a;">3V3</span></td>
      <td><span style="color:#000000;">Use 3.3V-safe peripherals</span></td>
    </tr>
    <tr>
      <td><span style="color:#1565c0;">Ground</span></td>
      <td><span style="color:#c62828;">GND</span></td>
      <td><span style="color:#6a1b9a;">GND</span></td>
      <td><span style="color:#000000;">Common ground for all devices</span></td>
    </tr>
  </tbody>
</table>

### Visual Wiring Diagram

```text
ESP32                          TFT (ILI9488 / ST7796 class)
-----                          -----------------------------
GPIO18  ---------------------> SCK
GPIO19  ---------------------> MISO
GPIO23  ---------------------> MOSI
GPIO14  ---------------------> CS
GPIO26  ---------------------> DC
GPIO4   ---------------------> RST
GPIO15  ---------------------> BL/LED
3V3     ---------------------> VCC
GND     ---------------------> GND

ESP32                          SHTC3 + VL6180X (shared I2C)
-----                          ----------------------------
GPIO21  ---------------------> SDA
GPIO22  ---------------------> SCL
GPIO27  ---------------------> VL6180X XSHUT
3V3     ---------------------> VCC
GND     ---------------------> GND
```

## Build/Flash (PlatformIO)

1. Open the repo with PlatformIO.
2. Review `platformio.ini` TFT driver settings:
   - Default is `ILI9488_DRIVER`.
   - If your panel is ST7796, switch that define accordingly.
3. Build and upload:
   - `pio run`
   - `pio run -t upload`
4. Open serial monitor:
   - `pio device monitor -b 115200`

## Local Web UI Tester (TFT Preview on localhost)

This repo includes a browser-based TFT mock so you can iterate on dashboard layout without flashing hardware.

### Option A: One-command launcher (PowerShell)

From repo root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\start-ui-tester.ps1 -Port 8765
```

Open:

- `http://localhost:8765`

### Option B: Manual static server

```powershell
cd .\ui-tester
python -m http.server 8765
```

Open:

- `http://localhost:8765`

### Tester Features

- 3-page UI flow with `NEXT PAGE` button:
  - Main Dashboard
  - Live Trends (Temp + Humidity chart)
  - SD Logging (clean table view, alternating row colors, save status)
- 480x320-style TFT frame with matching visual language.
- Temperature, humidity, and distance cards with live/offline badges and value bars.
- UI control panel to:
  - adjust sensor values
  - toggle SHTC3/VL6180X online states
  - toggle SD card installed state
  - enable/disable animation
- Simulated SD write interval is 3 seconds.

## UI Behavior

- The dashboard continuously updates:
  - Temperature card
  - Humidity card
  - Distance card
- Each card includes:
  - Current value
  - Online/offline badge
  - Compact trend sparkline
  - Scaled indicator bar
- Footer reports sensor health and sample age.

## Notes

- This project intentionally avoids control outputs for safety and clarity.
- If a sensor disconnects, UI marks it offline while preserving the app runtime.
- Tune scales and refresh rates in [`include/Config.h`](include/Config.h).
