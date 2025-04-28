#pragma once

#include <stdbool.h>

typedef struct {
    float pm1_0;
    float pm2_5;
    float pm4_0;
    float pm10_0;
    float humidity;
    float temperature;
    float voc_index;
    float nox_index;
    float co2_equivalent;
} sen66_data_t;

void sen66_i2c_init();
void sen66_start_measurement();
bool sen66_read_data(sen66_data_t *data);


