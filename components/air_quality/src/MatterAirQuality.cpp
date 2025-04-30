#include "MatterAirQuality.h"
#include <esp_log.h>
#include <common_macros.h>
#include <cmath>
#include <map>
#include "sen66_i2c.h"
#include "AirQualityClassifier.h"

// Sentinel values for invalid sensor data
constexpr uint16_t INVALID_UINT16 = 0xFFFF;
constexpr int16_t INVALID_INT16 = 0x7FFF;

static const char *TAG = "MatterAirQuality";

using namespace esp_matter;
using namespace chip::app::Clusters;

//------------------------------------------------------------------------------
// Helper to create a “ConcentrationMeasurement” cluster + its NumericMeasurement
//------------------------------------------------------------------------------
#define ADD_MEASUREMENT_CLUSTER(EspClusterNS, ChipCluster, MED, UNIT)                 \
  do {                                                                                \
    /* 1) build & create the base cluster */                                         \
    esp_matter::cluster::EspClusterNS::config_t cfg{};                                \
    cfg.measurement_medium =                                                           \
      static_cast<uint8_t>(ChipCluster::MeasurementMediumEnum::MED);                  \
    cluster_t * cl = esp_matter::cluster::EspClusterNS::create(                       \
      m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER                               \
    );                                                                                \
    ABORT_APP_ON_FAILURE( cl,                                                         \
      ESP_LOGE(TAG, "Failed to create " #EspClusterNS)                                \
    );                                                                                \
                                                                                      \
    /* 2) add the NumericMeasurement feature */                                 \
    esp_matter::cluster::EspClusterNS::feature::numeric_measurement::config_t num_cfg{}; \
    num_cfg.measurement_unit =                                                        \
      static_cast<uint8_t>(ChipCluster::MeasurementUnitEnum::UNIT);                   \
    esp_err_t err = esp_matter::cluster::EspClusterNS::feature::numeric_measurement::add( \
      cl, &num_cfg                                                                    \
    );                                                                                \
    if (err != ESP_OK) {                                                              \
      ESP_LOGW(TAG, #EspClusterNS " MEA unsupported (0x%X)", err);                    \
    }                                                                                 \
  } while (0)



MatterAirQuality::MatterAirQuality(node_t *node)
    : m_node(node), m_air_quality_endpoint(nullptr) {}

void MatterAirQuality::CreateAirQualityEndpoint()
{
    if (!InitializeEndpoint()) {
        ESP_LOGE(TAG, "Failed to initialize Air Quality endpoint");
        return;
    }

    AddAirQualityFeatures();
    AddStandardMeasurementClusters();
    AddCustomMeasurementClusters();

    ESP_LOGI(TAG, "Air Quality endpoint created successfully. Some features may not be supported and have been skipped.");
}

void MatterAirQuality::StartMeasurements()
{
    ESP_LOGI(TAG, "MatterAirQuality started");
}

void MatterAirQuality::SetFloatAttribute(uint16_t endpoint, uint32_t clusterId, uint32_t attributeId, float value)
{
    esp_matter_attr_val_t attrValue = esp_matter_float(value);
    attribute::update(endpoint, clusterId, attributeId, &attrValue);
}

bool MatterAirQuality::ReadSensor(sen66_data_t *out)
{
    return sen66_get_measurement(out);
}

void MatterAirQuality::UpdateAirQualityAttributes(const sen66_data_t *data)
{
    if (!m_air_quality_endpoint) {
        ESP_LOGW(TAG, "AQ endpoint not initialized");
        return;
    }

    uint16_t endpointId = endpoint::get_id(m_air_quality_endpoint);

    UpdateTemperatureAndHumidity(endpointId, data);
    UpdateConcentrationMeasurements(endpointId, data);
    UpdateAirQualityLevel(endpointId, data);
}

//------------------------------------------------------------------------------
// Private Methods
//------------------------------------------------------------------------------

bool MatterAirQuality::InitializeEndpoint()
{
    endpoint::air_quality_sensor::config_t ep_cfg = {};
    m_air_quality_endpoint = endpoint::air_quality_sensor::create(
        m_node, &ep_cfg, ENDPOINT_FLAG_DESTROYABLE, nullptr);

    if (!m_air_quality_endpoint) {
        ESP_LOGE(TAG, "Failed to create Air Quality endpoint");
        return false;
    }
    return true;
}

void MatterAirQuality::AddAirQualityFeatures()
{
    cluster_t *aq_cluster = cluster::get(m_air_quality_endpoint, AirQuality::Id);
    if (!aq_cluster) {
        ESP_LOGE(TAG, "Failed to get AirQuality cluster");
        return;
    }

    esp_matter::cluster::air_quality::feature::fair::add(aq_cluster);
    esp_matter::cluster::air_quality::feature::moderate::add(aq_cluster);
    esp_matter::cluster::air_quality::feature::very_poor::add(aq_cluster);
    esp_matter::cluster::air_quality::feature::extremely_poor::add(aq_cluster);
}

void MatterAirQuality::AddStandardMeasurementClusters()
{
    AddCluster<cluster::temperature_measurement::config_t>(
        cluster::temperature_measurement::create, "TemperatureMeasurement");

    AddCluster<cluster::relative_humidity_measurement::config_t>(
        cluster::relative_humidity_measurement::create, "RelativeHumidityMeasurement");
}

void MatterAirQuality::AddCustomMeasurementClusters()
{
    ADD_MEASUREMENT_CLUSTER(pm1_concentration_measurement, Pm1ConcentrationMeasurement, kAir, kUgm3);
    ADD_MEASUREMENT_CLUSTER(pm25_concentration_measurement, Pm25ConcentrationMeasurement, kAir, kUgm3);
    ADD_MEASUREMENT_CLUSTER(pm10_concentration_measurement, Pm10ConcentrationMeasurement, kAir, kUgm3);
    ADD_MEASUREMENT_CLUSTER(total_volatile_organic_compounds_concentration_measurement, TotalVolatileOrganicCompoundsConcentrationMeasurement, kAir, kPpm);
    ADD_MEASUREMENT_CLUSTER(carbon_dioxide_concentration_measurement, CarbonDioxideConcentrationMeasurement, kAir, kPpm);
    ADD_MEASUREMENT_CLUSTER(nitrogen_dioxide_concentration_measurement, NitrogenDioxideConcentrationMeasurement, kAir, kPpm);
}

template <typename ConfigType>
void MatterAirQuality::AddCluster(std::function<cluster_t *(endpoint_t *, ConfigType *, uint8_t)> createFunc, const char *name)
{
    ConfigType cfg = {};
    if (!createFunc(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER)) {
        ESP_LOGE(TAG, "Failed to create %s cluster", name);
    }
}

void MatterAirQuality::UpdateTemperatureAndHumidity(uint16_t endpointId, const sen66_data_t *data)
{
    if (!std::isnan(data->temperature)) {
        int16_t temp_val = static_cast<int16_t>(data->temperature * 100.0f); // .01°C units
        UpdateAttribute(endpointId, TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id, temp_val);
    }

    if (!std::isnan(data->humidity)) {
        int16_t hum_val = static_cast<int16_t>(data->humidity * 100.0f); // .01%RH units
        UpdateAttribute(endpointId, RelativeHumidityMeasurement::Id, RelativeHumidityMeasurement::Attributes::MeasuredValue::Id, hum_val);
    }
}

void MatterAirQuality::UpdateConcentrationMeasurements(uint16_t endpointId, const sen66_data_t *data)
{
    if (!std::isnan(data->co2_equivalent)) {
        SetFloatAttribute(endpointId, CarbonDioxideConcentrationMeasurement::Id,
                          CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id, data->co2_equivalent);
    }
    if (!std::isnan(data->pm1_0)) {
        SetFloatAttribute(endpointId, Pm1ConcentrationMeasurement::Id,
                          Pm1ConcentrationMeasurement::Attributes::MeasuredValue::Id, data->pm1_0);
    }
    if (!std::isnan(data->pm2_5)) {
        SetFloatAttribute(endpointId, Pm25ConcentrationMeasurement::Id,
                          Pm25ConcentrationMeasurement::Attributes::MeasuredValue::Id, data->pm2_5);
    }
    if (!std::isnan(data->pm10_0)) {
        SetFloatAttribute(endpointId, Pm10ConcentrationMeasurement::Id,
                          Pm10ConcentrationMeasurement::Attributes::MeasuredValue::Id, data->pm10_0);
    }
    if (!std::isnan(data->voc_index)) {
        SetFloatAttribute(endpointId, TotalVolatileOrganicCompoundsConcentrationMeasurement::Id,
                          TotalVolatileOrganicCompoundsConcentrationMeasurement::Attributes::MeasuredValue::Id, data->voc_index);
    }
    if (!std::isnan(data->nox_index)) {
        SetFloatAttribute(endpointId, NitrogenDioxideConcentrationMeasurement::Id,
                          NitrogenDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id, data->nox_index);
    }
}

void MatterAirQuality::UpdateAirQualityLevel(uint16_t endpointId, const sen66_data_t *data)
{
    AirQualityLevel level = AirQualityClassifier::classify(data);
    esp_matter_attr_val_t aq_index_val = esp_matter_int16(level);
    attribute::update(endpointId, AirQuality::Id, AirQuality::Attributes::AirQuality::Id, &aq_index_val);
}

template <typename T>
void MatterAirQuality::UpdateAttribute(uint16_t endpointId, uint32_t clusterId, uint32_t attributeId, T value)
{
    esp_matter_attr_val_t attrValue = esp_matter_int16(value);
    attribute::update(endpointId, clusterId, attributeId, &attrValue);
}
