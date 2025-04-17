#include "aq_sensor.h"

void aq_sensor_init(void) {
    // TODO: Initialize I2C + fake data for now
}

void aq_sensor_read(aq_measurement_t *result) {
    result->pm2_5 = 12.3;
    result->voc_index = 102.0;
    result->temperature = 23.5;
    result->humidity = 42.0;
}
