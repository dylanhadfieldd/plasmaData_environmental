# Appendix A: Detailed Components with Manufacturer Specifications

The exact vendor serial numbers are not documented in this repository, so the table uses manufacturer product IDs or controller part numbers where available. Rows marked as optional are supported or implied by the firmware, but should be confirmed against the physical build before final submission.

| Part | Serial Number/Product ID | Manufacturer | Description |
| --- | --- | --- | --- |
| ESP32 development board / microcontroller | ESP32-DevKitC V4 / ESP32-WROOM-32 series; firmware target: `esp32dev` | Espressif Systems | Main controller for the environmental monitoring dashboard. Provides GPIO headers, USB power/programming, Wi-Fi/Bluetooth capability, I2C for sensors, and SPI for the TFT display. Project wiring uses GPIO21/GPIO22 for I2C and GPIO18/GPIO19/GPIO23/GPIO14/GPIO26/GPIO4/GPIO15 for the TFT. |
| Temperature and humidity sensor | SHTC3 sensor IC; Adafruit SHTC3 STEMMA QT/Qwiic Product ID 4636 if using the Adafruit breakout | Sensirion for the sensor IC; Adafruit Industries for the breakout | Digital I2C temperature and relative humidity sensor. Measures 0-100% RH and -40 to 125 C, with typical accuracy of +/-2% RH and +/-0.2 C. Adafruit breakout uses I2C address 0x70 and supports 3 V or 5 V microcontroller interfaces through onboard support circuitry. |
| Time-of-flight distance sensor | VL6180X sensor IC; Adafruit VL6180X STEMMA QT Product ID 3316 if using the Adafruit breakout | STMicroelectronics for the sensor IC; Adafruit Industries for the breakout | I2C time-of-flight proximity/range sensor with ambient-light sensing. Measures absolute distance independent of target reflectance; Adafruit breakout is documented for about 5-100 mm range, with 150-200 mm possible under favorable conditions. Firmware uses the shared I2C bus and controls XSHUT on GPIO27. |
| TFT display module | ILI9488 3.5-inch SPI TFT; common LCDWiki module SKU MSP3520 with touch or MSP3521 without touch, if matching the physical module | ILI Technology Corp. for the ILI9488 controller IC; display module vendor varies | 3.5-inch color TFT display used for the dashboard UI. The project is configured for `ILI9488_DRIVER`, 480 x 320 pixel display class, 4-wire SPI, 3.3 V logic, and backlight control on GPIO15. README notes ST7796-class panels may also be used if the driver define is changed. |
| Capacitive touch controller, optional | FT62xx / FT6236 / FT6336 or GT911, depending on the installed TFT touch panel | FocalTech Systems for FT-series controllers; Goodix for GT911 | Optional I2C capacitive touch controller used for screen/page navigation if the display includes a capacitive touch panel. Firmware probes FT-series addresses 0x38 and 0x15 and GT911 addresses 0x5D and 0x14. Helper INT/RST pins are disabled in `Config.h`, so the current build uses I2C-only probing. |
| USB data and power cable | USB 2.0 Standard-A to Micro-B | Generic / varies | Supplies power and programming/serial communication for the ESP32 development board through the Micro-USB port. Required for flashing firmware and serial monitoring from the host computer. |
| Breadboard/jumper wiring harness | Dupont jumper wires; no unique product ID documented | Generic / varies | Provides the physical I2C, SPI, power, and ground connections described in the project wiring table. Use short, secure 3.3 V-compatible wiring for sensor and display signals. |

## Notes

- Current firmware is a sensor-only build and does not include physical SD-card logging pins or an SD library. The dashboard contains a CSV/logging preview, but a real SD card/module should not be listed as an active system component unless the physical hardware and firmware are added.
- If the physical TFT is an ST7796 module rather than ILI9488, replace the TFT display row with the module's actual driver IC and manufacturer information.
- If the SHTC3 or VL6180X sensors are not Adafruit breakouts, keep the sensor IC product IDs and replace the breakout product IDs with the actual purchased module identifiers.

## Sources

- Local project configuration and wiring: `platformio.ini`, `README.md`, `include/Config.h`, `src/SensorManager.cpp`, `src/UiRenderer.cpp`.
- Espressif ESP32-DevKitC V4 user guide: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html
- Sensirion SHTC3 product specification: https://sensirion.com/products/catalog/SHTC3
- Adafruit SHTC3 STEMMA QT/Qwiic Product ID 4636: https://www.adafruit.com/product/4636
- STMicroelectronics VL6180X product page: https://www.st.com/content/st_com/en/products/imaging-and-photonics-solutions/time-of-flight-sensors/vl6180x.html
- Adafruit VL6180X STEMMA QT Product ID 3316: https://www.adafruit.com/product/3316
- LCDWiki 3.5-inch SPI ILI9488 module information: https://www.lcdwiki.com/index.php?title=3.5inch_SPI_Module_ILI9488_SKU:MSP3520
- ILI9488 controller datasheet mirror: https://www.alldatasheet.com/html-pdf/1131840/ETC1/ILI9488/1881/16/ILI9488.html
- FT6236 controller datasheet summary: https://datasheet4u.com/datasheet-pdf/FocalTechSystems/FT6236/pdf.php?id=958370
- GT911 controller datasheet summary: https://datasheet4u.com/datasheets/GOODIX/GT911/1007756
