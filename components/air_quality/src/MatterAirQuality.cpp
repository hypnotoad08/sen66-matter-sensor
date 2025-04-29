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

using namespace esp_matter;
using namespace chip::app::Clusters;


static const char *TAG = "MatterAirQuality";

MatterAirQuality::MatterAirQuality(node_t *node)
  : m_node(node), m_air_quality_endpoint(nullptr)
{}

void MatterAirQuality::CreateAirQualityEndpoint()
{
    endpoint::air_quality_sensor::config_t ep_cfg = {};
    m_air_quality_endpoint = endpoint::air_quality_sensor::create(
        m_node, &ep_cfg, ENDPOINT_FLAG_DESTROYABLE, nullptr);
    ABORT_APP_ON_FAILURE(
        m_air_quality_endpoint,
        ESP_LOGE(TAG, "Failed to create Air Quality endpoint"));

    cluster_t *aq_cluster = cluster::get(m_air_quality_endpoint, AirQuality::Id);
        if (!aq_cluster) {
            ESP_LOGE(TAG, "Failed to get AirQuality cluster");
            return;
        }
        if (aq_cluster) {
            esp_matter::cluster::air_quality::feature::fair::add(aq_cluster);
            esp_matter::cluster::air_quality::feature::moderate::add(aq_cluster);
            esp_matter::cluster::air_quality::feature::very_poor::add(aq_cluster);
            esp_matter::cluster::air_quality::feature::extremely_poor::add(aq_cluster);
        }
        if (!aq_cluster) {
            ESP_LOGE(TAG, "Failed to get AirQuality cluster");
            return;
          }
          esp_matter::cluster::air_quality::feature::fair::add(aq_cluster);
          esp_matter::cluster::air_quality::feature::moderate ::add(aq_cluster);
          esp_matter::cluster::air_quality::feature::very_poor::add(aq_cluster);
          esp_matter::cluster::air_quality::feature::extremely_poor::add(aq_cluster);
        
    // 2) TemperatureMeasurement (INT16)
    {
        cluster::temperature_measurement::config_t cfg = {};
        ABORT_APP_ON_FAILURE(
            cluster::temperature_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER),
            ESP_LOGE(TAG, "Failed to create TemperatureMeasurement"));
    }

    // 3) RelativeHumidityMeasurement (INT16)
    {
        cluster::relative_humidity_measurement::config_t cfg = {};
        ABORT_APP_ON_FAILURE(
            cluster::relative_humidity_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER),
            ESP_LOGE(TAG, "Failed to create RelativeHumidityMeasurement"));
    }

    // Carbon Dioxide
    {
        cluster::carbon_dioxide_concentration_measurement::config_t cfg = {};
        cluster_t *co2_cluster = cluster::carbon_dioxide_concentration_measurement::create(
            m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        ABORT_APP_ON_FAILURE(co2_cluster, ESP_LOGE(TAG, "Failed to create CO2 cluster"));

        // <-- allocate a real config_t on the stack, then pass &num_cfg
        cluster::carbon_dioxide_concentration_measurement::feature::numeric_measurement::config_t num_cfg = {};
        esp_err_t err = cluster::carbon_dioxide_concentration_measurement::feature::numeric_measurement::add(
            co2_cluster, &num_cfg);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "CO2 NumericMeasurement unsupported (0x%X)", err);
        }
    }

    // PM1.0
    {
        cluster::pm1_concentration_measurement::config_t cfg = {};
        cluster_t *pm1_cluster = cluster::pm1_concentration_measurement::create(
            m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        ABORT_APP_ON_FAILURE(pm1_cluster, ESP_LOGE(TAG, "Failed to create PM1 cluster"));

        cluster::pm1_concentration_measurement::feature::numeric_measurement::config_t num_cfg = {};
        esp_err_t err = cluster::pm1_concentration_measurement::feature::numeric_measurement::add(
            pm1_cluster, &num_cfg);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "PM1 NumericMeasurement unsupported (0x%X)", err);
        }
    }

    // PM2.5
    {
        cluster::pm25_concentration_measurement::config_t cfg = {};
        cluster_t *pm25_cluster = cluster::pm25_concentration_measurement::create(
            m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        ABORT_APP_ON_FAILURE(pm25_cluster, ESP_LOGE(TAG, "Failed to create PM2.5 cluster"));

        cluster::pm25_concentration_measurement::feature::numeric_measurement::config_t num_cfg = {};
        esp_err_t err = cluster::pm25_concentration_measurement::feature::numeric_measurement::add(
            pm25_cluster, &num_cfg);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "PM2.5 NumericMeasurement unsupported (0x%X)", err);
        }
    }

    // PM10
    {
        cluster::pm10_concentration_measurement::config_t cfg = {};
        cluster_t *pm10_cluster = cluster::pm10_concentration_measurement::create(
            m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        ABORT_APP_ON_FAILURE(pm10_cluster, ESP_LOGE(TAG, "Failed to create PM10 cluster"));

        cluster::pm10_concentration_measurement::feature::numeric_measurement::config_t num_cfg = {};
        esp_err_t err = cluster::pm10_concentration_measurement::feature::numeric_measurement::add(
            pm10_cluster, &num_cfg);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "PM10 NumericMeasurement unsupported (0x%X)", err);
        }
    }

    // TVOC
    {
        cluster::total_volatile_organic_compounds_concentration_measurement::config_t cfg = {};
        cluster_t *voc_cluster = cluster::total_volatile_organic_compounds_concentration_measurement::create(
            m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        ABORT_APP_ON_FAILURE(voc_cluster, ESP_LOGE(TAG, "Failed to create TVOC cluster"));

        cluster::total_volatile_organic_compounds_concentration_measurement::feature::numeric_measurement::config_t num_cfg = {};
        esp_err_t err = cluster::total_volatile_organic_compounds_concentration_measurement::feature::numeric_measurement::add(
            voc_cluster, &num_cfg);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "TVOC NumericMeasurement unsupported (0x%X)", err);
        }
    }

    // NOx
    {
        cluster::nitrogen_dioxide_concentration_measurement::config_t cfg = {};
        cluster_t *nox_cluster = cluster::nitrogen_dioxide_concentration_measurement::create(
            m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        ABORT_APP_ON_FAILURE(nox_cluster, ESP_LOGE(TAG, "Failed to create NOx cluster"));

        cluster::nitrogen_dioxide_concentration_measurement::feature::numeric_measurement::config_t num_cfg = {};
        esp_err_t err = cluster::nitrogen_dioxide_concentration_measurement::feature::numeric_measurement::add(
            nox_cluster, &num_cfg);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "NOx NumericMeasurement unsupported (0x%X)", err);
        }
    }

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

void MatterAirQuality::UpdateAirQualityAttributes(const sen66_data_t *d)
{
    if (!m_air_quality_endpoint) {
        ESP_LOGW(TAG, "AQ endpoint not initialized");
        return;
    }
    uint16_t ep = endpoint::get_id(m_air_quality_endpoint);

    // INT16 clusters: Temperature & Humidity
    if (!std::isnan(d->temperature)) {
        int16_t temp_val = int16_t(d->temperature * 100.0f); // .01°C units
        esp_matter_attr_val_t v = esp_matter_int16(temp_val);
        attribute::update(
            ep,
            TemperatureMeasurement::Id,
            TemperatureMeasurement::Attributes::MeasuredValue::Id,
            &v
        );
    }
    if (!std::isnan(d->humidity)) {
        int16_t hum_val = int16_t(d->humidity * 100.0f); // .01%RH units
        esp_matter_attr_val_t v = esp_matter_int16(hum_val);
        attribute::update(
            ep,
            RelativeHumidityMeasurement::Id,
            RelativeHumidityMeasurement::Attributes::MeasuredValue::Id,
            &v
        );
    }


    if (!std::isnan(d->co2_equivalent)) {
        SetFloatAttribute(ep,CarbonDioxideConcentrationMeasurement::Id,
                 CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id,
                 d->co2_equivalent);
    }
    if (!std::isnan(d->pm1_0)) {
        SetFloatAttribute(ep,Pm1ConcentrationMeasurement::Id,
                 Pm1ConcentrationMeasurement::Attributes::MeasuredValue::Id,
                 d->pm1_0);
    }
    if (!std::isnan(d->pm2_5)) {
        SetFloatAttribute(ep,Pm25ConcentrationMeasurement::Id,
                 Pm25ConcentrationMeasurement::Attributes::MeasuredValue::Id,
                 d->pm2_5);
    }
    if (!std::isnan(d->pm10_0)) {
        SetFloatAttribute(ep,
            Pm10ConcentrationMeasurement::Id,
            Pm10ConcentrationMeasurement::Attributes::MeasuredValue::Id,
            d->pm10_0
        );
    }
    if (!std::isnan(d->voc_index)) {
        SetFloatAttribute(ep,TotalVolatileOrganicCompoundsConcentrationMeasurement::Id,
                 TotalVolatileOrganicCompoundsConcentrationMeasurement::Attributes::MeasuredValue::Id,
                 d->voc_index);
    }
    if (!std::isnan(d->nox_index)) {
        SetFloatAttribute(ep,NitrogenDioxideConcentrationMeasurement::Id,
                 NitrogenDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id,
                 d->nox_index);
    }

    AirQualityLevel lvl = AirQualityClassifier::classify(d);
    esp_matter_attr_val_t aq_index_val = esp_matter_int16(lvl);
    attribute::update(ep, AirQuality::Id,
                      AirQuality::Attributes::AirQuality::Id,
                      &aq_index_val);
}
