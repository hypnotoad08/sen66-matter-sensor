#include "sen66_sensor.h"
#include "sen66_i2c.h"
#include "sensirion_i2c_hal.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cmath>

static const char *TAG = "SEN66_SENSOR";
static constexpr TickType_t POLL_PERIOD = pdMS_TO_TICKS(50);
static constexpr TickType_t MAX_WAIT    = pdMS_TO_TICKS(500);
constexpr uint16_t INVALID_UINT16 = 0xFFFF;
constexpr int16_t INVALID_INT16 = 0x7FFF;

void sen66_i2c_init() {
    sensirion_i2c_hal_init();
    sen66_init(SEN66_I2C_ADDR_6B);
    sensirion_i2c_hal_sleep_usec(20000); // Wait 20ms after powering up
}

void sen66_start_measurement() {
    int16_t ret = sen66_start_continuous_measurement();
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to start continuous measurement, error: %d", ret);
    } else {
        ESP_LOGI(TAG, "Continuous measurement started");
    }
}

bool sen66_read_data(sen66_data_t *data) {

    uint8_t padding;
    bool ready;
    int ret = sen66_get_data_ready(&padding, &ready);
    if (ret != 0) {
        ESP_LOGW(TAG, "sen66_get_data_ready failed with error code %d", ret);
        return false;
    }
    if (!ready) {
        ESP_LOGW(TAG, "Sensor data not ready");
        return false;
    }

    uint16_t raw_pm1, raw_pm25, raw_pm4, raw_pm10, raw_co2;
    int16_t  raw_hum, raw_temp, raw_voc, raw_nox;

    sen66_read_measured_values_as_integers(
        &raw_pm1, &raw_pm25, &raw_pm4, &raw_pm10,
        &raw_hum, &raw_temp, &raw_voc, &raw_nox, &raw_co2);

        data->pm1_0        = (raw_pm1  != INVALID_UINT16) ? raw_pm1  / 10.0f : NAN;
        data->pm2_5        = (raw_pm25 != INVALID_UINT16) ? raw_pm25 / 10.0f : NAN;
        data->pm10_0       = (raw_pm10 != INVALID_UINT16) ? raw_pm10 / 10.0f : NAN;
        data->humidity     = (raw_hum  != INVALID_INT16) ? raw_hum  / 100.0f : NAN;
        data->temperature  = (raw_temp != INVALID_INT16) ? raw_temp / 200.0f : NAN;
        data->voc_index    = (raw_voc  != INVALID_INT16) ? raw_voc  / 10.0f : NAN;
        data->nox_index    = (raw_nox  != INVALID_INT16) ? raw_nox  / 10.0f : NAN;
        data->co2_equivalent = (raw_co2 != INVALID_UINT16) ? float(raw_co2) : NAN;

    return true;
}

bool sen66_get_measurement(sen66_data_t *out_data) {
    TickType_t elapsed = 0;
    while (elapsed < MAX_WAIT) {
        if (sen66_read_data(out_data)) {
            // ready flag was set and values decoded
            return true;
        }
        vTaskDelay(POLL_PERIOD);
        elapsed += POLL_PERIOD;
    }

    ESP_LOGW(TAG, "sen66_get_measurement: timeout waiting for data-ready");
    return false;
}