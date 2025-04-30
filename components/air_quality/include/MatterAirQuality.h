#pragma once

#include "sen66_sensor.h"
#include <esp_matter.h>

class MatterAirQuality {
public:
    MatterAirQuality(esp_matter::node_t *node);

    // Public methods
    void CreateAirQualityEndpoint();
    void StartMeasurements();
    bool ReadSensor(sen66_data_t *out);
    void SetFloatAttribute(uint16_t endpoint, uint32_t clusterId, uint32_t attributeId, float value);
    void UpdateAirQualityAttributes(const sen66_data_t *data);

private:
    // Private helper methods
    bool InitializeEndpoint();
    void AddAirQualityFeatures();
    void AddStandardMeasurementClusters();
    void AddCustomMeasurementClusters();

    template <typename ConfigType>
    void AddCluster(std::function<esp_matter::cluster_t *(esp_matter::endpoint_t *, ConfigType *, uint8_t)> createFunc, const char *name);

    template <typename T>
    void UpdateAttribute(uint16_t endpointId, uint32_t clusterId, uint32_t attributeId, T value);

    void UpdateTemperatureAndHumidity(uint16_t endpointId, const sen66_data_t *data);
    void UpdateConcentrationMeasurements(uint16_t endpointId, const sen66_data_t *data);
    void UpdateAirQualityLevel(uint16_t endpointId, const sen66_data_t *data);

    // Member variables
    esp_matter::node_t *m_node;
    esp_matter::endpoint_t *m_air_quality_endpoint = nullptr;
};