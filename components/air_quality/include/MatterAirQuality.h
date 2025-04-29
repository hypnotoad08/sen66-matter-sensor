#pragma once

#include "sen66_sensor.h"
#include <esp_matter.h>

class MatterAirQuality {
    public:
    MatterAirQuality(esp_matter::node_t *node);
    void CreateAirQualityEndpoint();
    void StartMeasurements();
    bool ReadSensor(sen66_data_t *out);
    void SetFloatAttribute(uint16_t endpoint, uint32_t clusterId, uint32_t attributeId, float value);
    void UpdateAirQualityAttributes(const sen66_data_t *data);

private:
    esp_matter::node_t *m_node;
    esp_matter::endpoint_t *m_air_quality_endpoint = nullptr;
    esp_matter::endpoint_t *m_endpoint_temp = nullptr;
    esp_matter::endpoint_t *m_endpoint_hum = nullptr;
    esp_matter::endpoint_t *m_endpoint_pm1 = nullptr;
    esp_matter::endpoint_t *m_endpoint_pm25 = nullptr;
    esp_matter::endpoint_t *m_endpoint_pm10 = nullptr;
    esp_matter::endpoint_t *m_endpoint_co2 = nullptr;
    esp_matter::endpoint_t *m_endpoint_voc = nullptr;
    esp_matter::endpoint_t *m_endpoint_nox = nullptr;
};