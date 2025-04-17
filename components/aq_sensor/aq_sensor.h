#pragma once

typedef struct {
    float pm2_5;
    float voc_index;
    float temperature;
    float humidity;
} aq_measurement_t;

void aq_sensor_init(void);
void aq_sensor_read(aq_measurement_t *result);
