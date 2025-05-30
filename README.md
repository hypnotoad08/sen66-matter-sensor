# SEN66 Matter Air Quality Sensor

“sen66-matter-sensor” is an ESP-IDF project that integrates the Sensirion SEN66 air-quality sensor with the Matter data model, exposing temperature, humidity, particulate-matter (PM₁/₂.₅/₁₀), CO₂, VOC, and NOₓ measurements as standard Matter clusters.

## 1. Board & Environment

- **Target**: Arduino Nano ESP32-S3  
- **ESP-IDF**: v5.4.1 or later  
- **Matter SDK**: ESP-Matter (via `esp_matter` component)

> **Note:** This project was built and tested on an **Arduino Nano ESP32-S3**. If you are using a different ESP32-based board (e.g., ESP32, ESP32-C3, ESP32-S2), please verify your `sdkconfig` matches your board's flash size, PSRAM configuration, and peripheral support before building/flashing.

- **I²C Pins**: The default I²C HAL in `components/sen66/src/sensirion_i2c_hal.c` sets:
```c
#define I2C_MASTER_SDA_IO 11  // SDA pin
#define I2C_MASTER_SCL_IO 12  // SCL pin
```
for the Arduino Nano ESP32. Update these definitions if using other boards.

### Prerequisites

1. Install ESP-IDF (https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/)  
2. Clone this repo and enter its folder:  
   ```bash
   git clone https://github.com/hypnotoad08/sen66-matter-sensor.git
   cd sen66-matter-sensor
   ```
3. Set environment variables:  
   ```bash
   export ESP_MATTER_PATH=/path/to/esp-matter
   export MATTER_SDK_PATH=/path/to/connectedhomeip
   ```
4. (Optional) Install Python3 requirements prompted by ESP-IDF.

## 2. Project Structure

```text
sen66-matter-sensor/
├── .devcontainer/            # VSCode devcontainer configuration
├── CMakeLists.txt            # Top-level project file
├── sdkconfig.defaults        # Default ESP-IDF config
├── partitions_example.csv    # Sample partition table (copy to partitions.csv)
├── docs/                     # Prebuilt docs (optional)
│   ├── factory_partition.bin # Factory partition image
│   └── qr_code.png           # QR code for Matter commissioning
├── main/                     # Main application component
│   ├── CMakeLists.txt
│   └── src/
│       ├── app_main.cpp      # Matter node init + sensor loop
│       ├── factory_reset.cpp # Button-triggered factory reset
│       └── sntp_sync.cpp     # SNTP time synchronization
├── components/               # Application components
│   ├── sen66/                # Sensirion SEN66 driver component
│   │   ├── include/          # Public headers
│   │   ├── src/              # I²C HAL & driver implementations
│   │   └── CMakeLists.txt
│   └── air_quality/          # Matter cluster glue
│       ├── include/          # `MatterAirQuality.h`
│       ├── src/              # Cluster implementation
│       └── CMakeLists.txt
├── .gitignore                # Git ignore rules
└── README.md                 # This file
```

## 3. Build & Flash

1. Source ESP-IDF:  
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```
2. Configure project:  
   ```bash
   idf.py set-target esp32s3
   idf.py menuconfig       # set serial port under "Serial flasher config"
   ```
3. Prepare partition table:  
   ```bash
   cp partitions_example.csv partitions.csv
   ```
4. Build, flash, and monitor:  
   ```bash
   idf.py -p /dev/ttyUSB0 build flash monitor
   ```

### Custom Vendor & Manufacturing Info

To program custom Matter vendor/product details, use `esp-matter-mfg-tool`:

```bash
esp-matter-mfg-tool   -cn "SEN66 Matter Air Sensor"   -v 0xFFF2 --vendor-name "Lee Dev"   -p 0x66A1 --product-name "Nano AQ Sensor"   --pai --serial-num SN-ESP32S3-0001   --hw-ver 1 --hw-ver-str v1.0   -k path/to/test-PAI-0xFFF2-key.pem   -c path/to/test-PAI-0xFFF2-cert.pem   -cd path/to/Chip-Test-CD-0xFFF2-0x66A1.der
```

### Reverting to Built-in ESP32 DAC

To restore the default (Test) DAC setup in ESP Matter using your checked-in `sdkconfig`:

1. **Menuconfig**  
   ```bash
   idf.py menuconfig
   ```
   Navigate to:  
   ```
   Component config → ESP Matter → Factory Data Providers → DAC Provider options
   ```
   and select **Attestation - Test** (instead of `Factory Partition`).

2. **SDKconfig defaults**  
   In your `sdkconfig.defaults` (or `sdkconfig`), override the factory data settings to disable the ESP32 factory provider and partition-backed DAC:
   ```ini
   # Disable reading from factory partition
   CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER=n
   CONFIG_FACTORY_PARTITION_DAC_PROVIDER=n

   # Disable other factory info providers if present
   CONFIG_FACTORY_COMMISSIONABLE_DATA_PROVIDER=n
   CONFIG_FACTORY_DEVICE_INSTANCE_INFO_PROVIDER=n
   ```
3. **Partition table**  
   If you have a `chip-factory` partition in your `partitions.csv`, remove its entry:
   ```text
   chip-factory,   data, nvs, 0x1A000,   0x06000,
   ```
      Then save and flash with the updated table.

This ensures the firmware falls back to the built-in ESP32 DAC provider rather than using the factory partition.

## 4. Matter Commissioning & Pairing

1. After flashing, the device will join Wi-Fi (check serial logs for IP)  
2. Scan `docs/qr_code.png` in your Matter controller app to commission  
3. Or use `chip-tool` for on-network commissioning:
   ```bash
   chip-tool pairing onnetwork-setup-code 20202021 <DEVICE_IP> 5540
   ```
   - Default setup PIN: **20202021**  
   - Matter port: **5540**  
4. Read attributes, e.g.:
   ```bash
   chip-tool read attribute 1 0x0402 0x0000
   ```

### Embedded QR Code & Hard-coded Pairing Info

The following QR code and manual pairing data are _hard-coded_ to match the provided `chip-factory` partition. **If you revert to the Test (built-in) DAC provider or generate your own credentials with `esp-matter-mfg-tool`, replace these values accordingly.**

![Matter QR Code](docs/qr_code.png)

| QR Code                | Manual Code   | Discriminator | Pairing Code |
|------------------------|---------------|--------------:|-------------:|
| MT:634J0VJF15BJ3706L10 | 0171-444-4965 |           397 |     73663224 |

## 5. Usage & Behavior

- **Altitude Compensation**  
  At startup we set the SEN66 sensor altitude (defaults to 0 m). For example, Cleveland, OH is approximately **206 m** above sea level for CO₂ pressure compensation.

- **Invalid-Reading Protection**  
  Any raw “sentinel” values (`0x7FFF`, `0xFFFF`) are converted to `NaN`. If *any* channel in a cycle is non-finite, that entire cycle is skipped.

- **Smoothing**  
  We apply a 5-sample Simple Moving Average to PM₁.₀, PM₂.₅ and PM₁₀ readings before running change-threshold logic.

- **Change-Threshold Reporting**  
  Matter attributes are only updated when the change vs. the last published reading exceeds these thresholds:

  | Parameter       | Threshold |
  | --------------- | --------- |
  | PM₁.₀           | 1 µg/m³   |
  | PM₂.₅           | 1 µg/m³   |
  | PM₁₀            | 1 µg/m³   |
  | CO₂             | 50 ppm    |
  | VOC             | 10 ppb    |
  | NOₓ             | 5 ppb     |
  | Temperature     | 0.5 °C    |
  | Humidity        | 2 %RH     |

- **State Persistence**  
  The last published sensor values are stored in NVS and restored across resets, avoiding jumps or stale data when the device restarts.

- **Measurement Loop (every 5 s)**  
  1. Initiate a SEN66 measurement  
  2. Filter out sentinel values → `NaN`  
  3. Convert raw → real-world units  
  4. Apply SMA smoothing to PM readings  
  5. Compare deltas vs. thresholds  
  6. Update only the Matter `MeasuredValue` attributes that exceed thresholds  

**Supported Matter clusters:**  
- TemperatureMeasurement  
- RelativeHumidityMeasurement  
- Pm1ConcentrationMeasurement  
- Pm25ConcentrationMeasurement  
- Pm10ConcentrationMeasurement  
- CarbonDioxideConcentrationMeasurement  
- TotalVolatileOrganicCompoundsConcentrationMeasurement  
- NitrogenDioxideConcentrationMeasurement  

## 6. Dependencies

- Sensirion I²C SEN66 driver: https://github.com/Sensirion/embedded-i2c-sen66  
- ESP-Matter (`esp_matter` component)  
- ESP-IDF components: `nvs_flash`, `esp_timer`  

## 7. Further Reading

- **ESP-Matter**: https://github.com/espressif/esp_matter  
- **Connected Home over IP**: https://github.com/project-chip/connectedhomeip  
- **ESP-IDF Guide**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/  

## 8. Code Annotations

In your `app_main.cpp`, the `displayMatterInfo()` function currently uses hard-coded values that match your `chip-factory` data:

```cpp
payload.discriminator.SetLongValue(397);
payload.setUpPINCode = 73663224;
```

These values are derived from the factory partition's metadata. If you revert to the Test (built-in) DAC provider and remove the `chip-factory` partition, you must:

- Update `discriminator` and `setUpPINCode` to the new values defined (e.g., from `sdkconfig` or code`)  
- Or remove/disable the `displayMatterInfo()` call if not needed.

Adjust these constants to match your commissioning data for accurate QR/manual pairing information.

*Inspired by [matter-air-quality-esp32](https://github.com/olavt/matter-air-quality-esp32) by @olavt.*

© 2025 Lee Dev · MIT License