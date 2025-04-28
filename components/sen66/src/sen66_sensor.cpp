#include "sen66_sensor.h"
#include "sen66_i2c.h"
#include "sensirion_i2c_hal.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "SEN66_SENSOR";

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
    uint16_t pm1p0 = 0;
    uint16_t pm2p5 = 0;
    uint16_t pm4p0 = 0; // <- We can discard if unused
    uint16_t pm10p0 = 0;
    int16_t humidity = 0;
    int16_t temperature = 0;
    int16_t voc = 0;
    int16_t nox = 0;
    uint16_t co2 = 0;

    int16_t ret = sen66_read_measured_values_as_integers(
        &pm1p0, &pm2p5, &pm4p0, &pm10p0,
        &humidity, &temperature, &voc, &nox, &co2);

    if (ret != 0) {
        ESP_LOGE(TAG, "sen66_read_measured_values_as_integers failed: %d", ret);
        return false;
    }

    data->pm1_0 = pm1p0 / 10.0f;      // PM values scaled by 10
    data->pm2_5 = pm2p5 / 10.0f;
    data->pm10_0 = pm10p0 / 10.0f;
    data->humidity = humidity / 100.0f;      // Humidity scaled by 100
    data->temperature = temperature / 200.0f; // Temperature scaled by 200
    data->voc_index = voc / 10.0f;            // VOC scaled by 10
    data->nox_index = nox / 10.0f;            // NOx scaled by 10
    data->co2_equivalent = co2;               // CO2 already in ppm

    return true;
}