#pragma once

#include <stdbool.h>
#include <cstdint>

struct sen66_data_t {
    int16_t  raw_temperature;      // 0..0x7FFE valid, 0x7FFF = invalid
    float    temperature;         // = raw_temperature / 200.0f
    int16_t  raw_humidity;        // 0..0x7FFE valid, 0x7FFF = invalid
    float    humidity;            // = raw_humidity / 100.0f
    uint16_t raw_pm1_0;           // 0..0xFFFE valid, 0xFFFF = invalid
    float    pm1_0;               // = raw_pm1_0 / 10.0f
    uint16_t raw_pm2_5;           // same
    float    pm2_5;
    uint16_t raw_pm10_0;
    float    pm10_0;
    int16_t  raw_voc_index;       // 0..0x7FFE valid, 0x7FFF = invalid
    float    voc_index;           // = raw_voc_index / 10.0f
    int16_t  raw_nox_index;       // same as VOC
    float    nox_index;
    uint16_t raw_co2;             // 0..0xFFFE valid, 0xFFFF = invalid
    float    co2_equivalent;      // = raw_co2
};

void sen66_i2c_init();
bool sen66_get_measurement(sen66_data_t *out_data);
void sen66_start_measurement();
bool sen66_read_data(sen66_data_t *data);


