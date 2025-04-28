# sen66-matter-sensor

"sen66-matter-sensor" is an ESP-IDF project that integrates the Sensirion SEN66 air quality sensor with the ESP Matter data model, exposing temperature, humidity, particulate matter, CO₂, VOC, and NOₓ measurements as Matter clusters.

## 1. Board & Environment

- **Target**: Arduino Nano ESP32 (ESP32-S3)
- **ESP-IDF**: v5.4.1 or later
- **Matter SDK**: ESP Matter (via `esp_matter` component)

### Prerequisites

1. Install ESP-IDF (https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/).
2. Clone this repository:
   ```bash
   git clone https://github.com/hypnotoad08/sen66-matter-sensor.git
   cd sen66-matter-sensor
   ```
3. Set `ESP_MATTER_PATH` and `MATTER_SDK_PATH` in your environment to point at your `esp-matter` and `connectedhomeip` checkout locations, respectively.
4. Install Python3 requirements if prompted by ESP-IDF.

## 2. Project Structure

```
sen66-matter-sensor/
├── CMakeLists.txt            # Top-level project settings
├── sdkconfig.defaults        # Default IDF configuration for board
├── main/
│   ├── src/
│   │   ├── app_main.cpp      # Initializes Matter node, sensor loop
│   │   ├── factory_reset.cpp # Button-driven factory reset logic
│   │   └── sntp_sync.cpp     # SNTP setup for timestamping
│   └── CMakeLists.txt        # IDF component for main application
├── components/
│   ├── sen66/                # Sensirion SEN66 driver component
│   │   ├── include/          # Public headers for `sensirion_*` APIs
│   │   ├── src/              # I²C HAL and common implementations
│   │   └── CMakeLists.txt
│   └── air_quality/          # Matter cluster glue for air quality
│       ├── include/          # `MatterAirQuality.h`
│       ├── src/              # `MatterAirQuality.cpp` updates Matter attributes
│       └── CMakeLists.txt
└── README.md
```

## 3. Build & Flash

```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Configure project
idf.py menuconfig  # choose your serial port under "Serial flasher config"

# Build
idf.py build

# Flash to device
idf.py -p /dev/ttyUSB0 flash monitor
```

## 4. Usage

1. Power on your Arduino Nano ESP32 with SEN66 sensor wired to I²C.
2. Upon boot, the device will:
   - Initialize Matter node
   - Create an AirQualitySensor endpoint
   - Start continuous SEN66 measurements every 5 s
3. Discover the device using any Matter controller; observe clusters:
   - TemperatureMeasurement
   - RelativeHumidityMeasurement
   - Pm1/2.5/10ConcentrationMeasurement
   - CarbonDioxideConcentrationMeasurement
   - TotalVolatileOrganicCompoundsConcentrationMeasurement
   - NitrogenDioxideConcentrationMeasurement
4. Read `MeasuredValue` attributes to get real‑time sensor data.

## 5. Notes

- The code filters out SEN66 sentinel values (`0x7FFF` for INT16, `0xFFFF` for UINT16) and reports invalid readings as `NAN` or skips updates.
- Adjust the update interval by modifying `vTaskDelay(pdMS_TO_TICKS(5000))` in `app_main.cpp`.

---

*Happy sensing!*

