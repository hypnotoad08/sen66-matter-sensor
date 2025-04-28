# SEN66 Matter Air Quality Sensor

“sen66-matter-sensor” is an ESP-Matter project that integrates the Sensirion SEN66 air-quality sensor with the Matter data model, exposing temperature, humidity, particulate-matter (PM₁/₂.₅/₁₀), CO₂, VOC, and NOₓ measurements as standard Matter clusters.

---

## 1. Board & Environment

- **Target**: Arduino Nano ESP32-S3  
- **ESP-IDF**: v5.4.1 or later  
- **Matter SDK**: ESP-Matter (via `esp_matter` component)  

### Prerequisites

1. Install ESP-IDF (see https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/).  
2. Clone this repo and enter its folder:
   ```bash
   git clone https://github.com/hypnotoad08/sen66-matter-sensor.git
   cd sen66-matter-sensor
   ```
3. Set environment variables:
   - `ESP_MATTER_PATH` → path to your `esp-matter` checkout  
   - `MATTER_SDK_PATH` → path to your `connectedhomeip` checkout  
4. (Optional) Install any Python3 requirements automatically prompted by ESP-IDF.

---

## 2. Project Structure

```
sen66-matter-sensor/
├── CMakeLists.txt            # Top-level project file
├── sdkconfig.defaults        # Default IDF config (board, flash, etc.)
├── docs/
│   ├── factory_partition.bin # Prebuilt factory partition
│   └── qr_code.png           # Matter commissioning QR code
├── main/
│   ├── CMakeLists.txt        # Main app component
│   └── src/
│       ├── app_main.cpp      # Matter node + sensor loop
│       ├── factory_reset.cpp # Button-triggered factory reset
│       └── sntp_sync.cpp     # SNTP time sync
├── components/
│   ├── sen66/                # Sensirion I²C driver
│   │   ├── include/          # `sensirion_*` headers
│   │   ├── src/              # HAL + common C files
│   │   └── CMakeLists.txt
│   └── air_quality/          # Matter cluster glue
│       ├── include/          # `MatterAirQuality.h`
│       ├── src/              # `MatterAirQuality.cpp`
│       └── CMakeLists.txt
└── README.md                 # This file
```

---

## 3. Build & Flash

```bash
# 1) Source ESP-IDF
. $HOME/esp/esp-idf/export.sh

# 2) Configure
idf.py set-target esp32s3
idf.py menuconfig           # set your serial port under "Serial flasher config"

# 3) Build & flash + monitor
idf.py -p /dev/ttyUSB0 build flash monitor
```

> **Custom factory partition & QR code**  
> Prebuilt `factory_partition.bin` and `qr_code.png` live in `docs/`.  
> To generate your own, see esp-matter-mfg-tool docs:  
> https://github.com/espressif/esp_matter/tree/master/tools/esp-matter-mfg-tool

---

## 4. Matter Commissioning & Pairing

1. After flash, device joins Wi-Fi (check serial logs for IP).  
2. Scan `docs/qr_code.png` in your Matter controller app.  
3. Or use `chip-tool` on-network commissioning:
   ```
   chip-tool pairing onnetwork-setup-code 20202021 <DEVICE_IP> 5540
   ```
   - Default setup PIN: **20202021**  
   - Matter port: **5540**

4. Read attributes, e.g.:
   ```
   chip-tool read attribute 1 0x0402 0x0000
   ```

---

## 5. Usage & Behavior

- Every **5 s**, SEN66 measures and the firmware:
  - Filters out sentinel values (`0x7FFF`, `0xFFFF`)
  - Converts raw → real units
  - Updates only valid Matter cluster `MeasuredValue` attributes

- Implemented clusters:
  - `TemperatureMeasurement`
  - `RelativeHumidityMeasurement`
  - `Pm1ConcentrationMeasurement`
  - `Pm25ConcentrationMeasurement`
  - `Pm10ConcentrationMeasurement`
  - `CarbonDioxideConcentrationMeasurement`
  - `TotalVolatileOrganicCompoundsConcentrationMeasurement`
  - `NitrogenDioxideConcentrationMeasurement`

---

## 6. Sensor Driver

Using Sensirion’s official embedded‐I²C SEN66 driver:

https://github.com/Sensirion/embedded-i2c-sen66

---

## 7. Further Reading

- **ESP-Matter**: https://github.com/espressif/esp_matter  
- **Connected Home over IP**: https://github.com/project-chip/connectedhomeip  
- **ESP-IDF Guide**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/  

---

© 2025 Lee Dev · MIT License
