#include "MatterAirQuality.h"
#include <esp_log.h>
#include <common_macros.h>
#include <cmath>
#include <map>
#include "sen66_i2c.h"
#include "AirQualityClassifier.h"
#include "SEN66Ranges.h"
#include <esp_matter.h>
#include <esp_matter_attribute.h>
#include <esp_matter.h>
#include <esp_matter_cluster.h>
#include <esp_matter_feature.h>


// Sentinel values for invalid sensor data
constexpr uint16_t INVALID_UINT16 = 0xFFFF;
constexpr int16_t INVALID_INT16 = 0x7FFF;

static const char *TAG = "MatterAirQuality";

using namespace esp_matter;
using namespace chip::app::Clusters;

//------------------------------------------------------------------------------
// Helper to create a “ConcentrationMeasurement” cluster + its NumericMeasurement
//------------------------------------------------------------------------------
#define ADD_MEASUREMENT_CLUSTER(EspClusterNS, ChipCluster, MED, UNIT, MIN_VAL, MAX_VAL)                 \
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
    num_cfg.min_measured_value =(MIN_VAL); \
    num_cfg.max_measured_value =(MAX_VAL); \
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
    using namespace sen66_ranges;
    AddCluster<cluster::temperature_measurement::config_t>(
        cluster::temperature_measurement::create, "TemperatureMeasurement", TEMP_MIN, TEMP_MAX); 

    AddCluster<cluster::relative_humidity_measurement::config_t>(
        cluster::relative_humidity_measurement::create, "RelativeHumidityMeasurement", HUM_MIN, HUM_MAX);
}

void MatterAirQuality::AddCustomMeasurementClusters()
{
    using namespace sen66_ranges;
    ADD_MEASUREMENT_CLUSTER(pm1_concentration_measurement, Pm1ConcentrationMeasurement, kAir, kUgm3,PM_MIN, PM_MAX);
    ADD_MEASUREMENT_CLUSTER(pm25_concentration_measurement, Pm25ConcentrationMeasurement, kAir, kUgm3,PM_MIN, PM_MAX);
    ADD_MEASUREMENT_CLUSTER(pm10_concentration_measurement, Pm10ConcentrationMeasurement, kAir, kUgm3,PM_MIN, PM_MAX);
    ADD_MEASUREMENT_CLUSTER(total_volatile_organic_compounds_concentration_measurement, TotalVolatileOrganicCompoundsConcentrationMeasurement, kAir, kPpm,VOC_MIN, VOC_MAX);    
    ADD_MEASUREMENT_CLUSTER(carbon_dioxide_concentration_measurement, CarbonDioxideConcentrationMeasurement, kAir, kPpm,ECO2_MIN, ECO2_MAX);
    ADD_MEASUREMENT_CLUSTER(nitrogen_dioxide_concentration_measurement, NitrogenDioxideConcentrationMeasurement, kAir, kPpm,NOX_MIN, NOX_MAX);
}

template <typename ConfigType>
void MatterAirQuality::AddCluster(std::function<cluster_t *(endpoint_t *, ConfigType *, uint8_t)> createFunc, const char *name, float minValue, float maxValue)
{
    ConfigType cfg{};
    cfg.min_measured_value = minValue;
    cfg.max_measured_value = maxValue;

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
        UpdateAttribute(endpointId, CarbonDioxideConcentrationMeasurement::Id,
                        CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id, data->co2_equivalent);
    }
    if (!std::isnan(data->pm1_0)) {
        UpdateAttribute(endpointId, Pm1ConcentrationMeasurement::Id,
                        Pm1ConcentrationMeasurement::Attributes::MeasuredValue::Id, data->pm1_0);
    }
    if (!std::isnan(data->pm2_5)) {
        UpdateAttribute(endpointId, Pm25ConcentrationMeasurement::Id,
                        Pm25ConcentrationMeasurement::Attributes::MeasuredValue::Id, data->pm2_5);
    }
    if (!std::isnan(data->pm10_0)) {
        UpdateAttribute(endpointId, Pm10ConcentrationMeasurement::Id,
                        Pm10ConcentrationMeasurement::Attributes::MeasuredValue::Id, data->pm10_0);
    }
    if (!std::isnan(data->voc_index)) {
        UpdateAttribute(endpointId, TotalVolatileOrganicCompoundsConcentrationMeasurement::Id,
                        TotalVolatileOrganicCompoundsConcentrationMeasurement::Attributes::MeasuredValue::Id, data->voc_index);
    }
    if (!std::isnan(data->nox_index)) {
        UpdateAttribute(endpointId, NitrogenDioxideConcentrationMeasurement::Id,
                        NitrogenDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id, data->nox_index);
    }
}

void MatterAirQuality::UpdateAirQualityLevel(uint16_t endpointId, const sen66_data_t *data)
{
    AirQualityLevel level = AirQualityClassifier::classify(data);
    UpdateAttribute(endpointId, AirQuality::Id, AirQuality::Attributes::AirQuality::Id, static_cast<int16_t>(level));
}

//------------------------------------------------------------------------------
// Attribute Update Helper
//------------------------------------------------------------------------------
template<typename T>
void MatterAirQuality::UpdateAttribute(uint16_t endpointId,
                                       uint32_t clusterId,
                                       uint32_t attributeId,
                                       T newValue)
{
    attribute_t *attr = attribute::get(endpointId, clusterId, attributeId);
    if (!attr) {
        ESP_LOGW(TAG, "Attr not found ep=%u cl=0x%08X attr=0x%08X",
                 static_cast<unsigned>(endpointId),
                 static_cast<unsigned>(clusterId),
                 static_cast<unsigned>(attributeId));
        return;
    }

    esp_matter_attr_val_t currentVal{};
    if (attribute::get_val(attr, &currentVal) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read current attribute value");
        return;
    }

    T oldValue;
    if constexpr (std::is_same_v<T, float>) {
        oldValue = currentVal.val.f;
    } else if constexpr (std::is_same_v<T, int16_t>) {
        oldValue = currentVal.val.i16;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        oldValue = currentVal.val.u16;
    } else if constexpr (std::is_same_v<T, int32_t>) {
        oldValue = currentVal.val.i32;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        oldValue = currentVal.val.u32;
    } else if constexpr (std::is_same_v<T, bool>) {
        oldValue = currentVal.val.b;
    } else {
        oldValue = static_cast<T>(currentVal.val.u32);
    }

    if (oldValue == newValue) {
        return;
    }

    esp_matter_attr_val_t toWrite{};
    if constexpr (std::is_same_v<T, float>) {
        toWrite = esp_matter_float(newValue);
    } else if constexpr (std::is_same_v<T, int16_t>) {
        toWrite = esp_matter_int16(newValue);
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        toWrite = esp_matter_uint16(newValue);
    } else if constexpr (std::is_same_v<T, int32_t>) {
        toWrite = esp_matter_int(newValue);
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        toWrite = esp_matter_uint32(newValue);
    } else if constexpr (std::is_same_v<T, bool>) {
        toWrite = esp_matter_bool(newValue);
    } else {
        toWrite = esp_matter_uint32(static_cast<uint32_t>(newValue));
    }

    attribute::update(endpointId, clusterId, attributeId, &toWrite);
}
