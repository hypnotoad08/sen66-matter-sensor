#include "MatterAirQuality.h"
#include <esp_log.h>
#include <common_macros.h>
#include <cmath>
#include <inttypes.h>

using namespace esp_matter;
using namespace chip::app::Clusters;

static const char *TAG = "MatterAirQuality";

MatterAirQuality::MatterAirQuality(esp_matter::node_t *node)
{
    m_node = node;
}

void MatterAirQuality::CreateAirQualityEndpoint()
{
    esp_matter::endpoint::air_quality_sensor::config_t config;
    uint8_t flags = esp_matter::ENDPOINT_FLAG_DESTROYABLE;
    m_air_quality_endpoint = esp_matter::endpoint::air_quality_sensor::create(m_node, &config, flags, nullptr);
    if (!m_air_quality_endpoint) {
        ESP_LOGE(TAG, "Failed to create Air Quality endpoint");
        return;
    }

    // Add Air Quality Cluster (base cluster) - no extra features needed

    // TemperatureMeasurement Cluster
    {
        cluster::temperature_measurement::config_t cfg = {};
        cluster::temperature_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
    }

    // RelativeHumidityMeasurement Cluster
    {
        cluster::relative_humidity_measurement::config_t cfg = {};
        cluster::relative_humidity_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
    }

    // CarbonDioxideConcentrationMeasurement Cluster
    {
        cluster::carbon_dioxide_concentration_measurement::config_t cfg = {};
        cluster_t *co2_cluster = cluster::carbon_dioxide_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        if (co2_cluster) {
            cluster::carbon_dioxide_concentration_measurement::feature::numeric_measurement::config_t feature_cfg = {};
            cluster::carbon_dioxide_concentration_measurement::feature::numeric_measurement::add(co2_cluster, &feature_cfg);
        }
    }

    // PM1ConcentrationMeasurement Cluster
    {
        cluster::pm1_concentration_measurement::config_t cfg = {};
        cluster_t *pm1_cluster = cluster::pm1_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        if (pm1_cluster) {
            cluster::pm1_concentration_measurement::feature::numeric_measurement::config_t feature_cfg = {};
            cluster::pm1_concentration_measurement::feature::numeric_measurement::add(pm1_cluster, &feature_cfg);
        }
    }

    // PM2.5ConcentrationMeasurement Cluster
    {
        cluster::pm25_concentration_measurement::config_t cfg = {};
        cluster_t *pm25_cluster = cluster::pm25_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        if (pm25_cluster) {
            cluster::pm25_concentration_measurement::feature::numeric_measurement::config_t feature_cfg = {};
            cluster::pm25_concentration_measurement::feature::numeric_measurement::add(pm25_cluster, &feature_cfg);
        }
    }

    // PM10ConcentrationMeasurement Cluster
    {
        cluster::pm10_concentration_measurement::config_t cfg = {};
        cluster_t *pm10_cluster = cluster::pm10_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        if (pm10_cluster) {
            cluster::pm10_concentration_measurement::feature::numeric_measurement::config_t feature_cfg = {};
            cluster::pm10_concentration_measurement::feature::numeric_measurement::add(pm10_cluster, &feature_cfg);
        }
    }

    // TotalVolatileOrganicCompoundsConcentrationMeasurement Cluster
    {
        cluster::total_volatile_organic_compounds_concentration_measurement::config_t cfg = {};
        cluster_t *voc_cluster = cluster::total_volatile_organic_compounds_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        if (voc_cluster) {
            cluster::total_volatile_organic_compounds_concentration_measurement::feature::numeric_measurement::config_t feature_cfg = {};
            cluster::total_volatile_organic_compounds_concentration_measurement::feature::numeric_measurement::add(voc_cluster, &feature_cfg);
        }
    }

    // NitrogenDioxideConcentrationMeasurement Cluster
    {
        cluster::nitrogen_dioxide_concentration_measurement::config_t cfg = {};
        cluster_t *nox_cluster = cluster::nitrogen_dioxide_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
        if (nox_cluster) {
            cluster::nitrogen_dioxide_concentration_measurement::feature::numeric_measurement::config_t feature_cfg = {};
            cluster::nitrogen_dioxide_concentration_measurement::feature::numeric_measurement::add(nox_cluster, &feature_cfg);
        }
    }
}

void MatterAirQuality::StartMeasurements()
{
    ESP_LOGI(TAG, "MatterAirQuality started");
}

static bool AttributeNeedsUpdate(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t new_val)
{   
    cluster_t *cluster = esp_matter::cluster::get(endpoint_id, cluster_id);
    if (!cluster) {
        ESP_LOGW(TAG, "Cluster 0x%08" PRIX32 " not found on endpoint 0x%04X", cluster_id, endpoint_id);
        return false; // Nothing to update
    }
    
    attribute_t *attr = esp_matter::attribute::get(cluster, attribute_id);
    if (!attr) {
        ESP_LOGW(TAG, "Attribute 0x%08" PRIX32 " not found in cluster 0x%08" PRIX32, attribute_id, cluster_id);
        return false; // Nothing to update
    }
    

    esp_matter_attr_val_t current_val = {};
    esp_err_t err = esp_matter::attribute::get_val(attr, &current_val);
    if (err != ESP_OK) {
         ESP_LOGW(TAG, "Failed to read attribute 0x%08" PRIX32 " (err=0x%x)", attribute_id, err);
        return true; // Assume update needed if we can't read it
    }

    if (current_val.type != new_val.type) {
        return true; // Type mismatch, better update
    }

    switch (new_val.type) {
        case ESP_MATTER_VAL_TYPE_INT16:
            return current_val.val.i16 != new_val.val.i16;
        case ESP_MATTER_VAL_TYPE_FLOAT:
            return fabs(current_val.val.f - new_val.val.f) > 0.01f; // Tiny tolerance for floats
        default:
            return true; // Unknown types, safer to update
    }
}


void MatterAirQuality::UpdateAirQualityAttributes(const sen66_data_t *data)
{
    if (!m_air_quality_endpoint) {
        ESP_LOGW(TAG, "Air Quality endpoint not initialized");
        return;
    }

    const uint16_t endpoint_id = esp_matter::endpoint::get_id(m_air_quality_endpoint);

    struct Int16Update {
        uint32_t cluster_id;
        uint32_t attribute_id;
        int16_t value;
    };

    struct FloatUpdate {
        uint32_t cluster_id;
        uint32_t attribute_id;
        float value;
    };

    Int16Update int16_updates[] = {
        { TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id, static_cast<int16_t>(data->temperature * 100) },
        { RelativeHumidityMeasurement::Id, RelativeHumidityMeasurement::Attributes::MeasuredValue::Id, static_cast<int16_t>(data->humidity * 100) },
        { CarbonDioxideConcentrationMeasurement::Id, CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id, static_cast<int16_t>(data->co2_equivalent) }
    };

    FloatUpdate float_updates[] = {
        { Pm1ConcentrationMeasurement::Id, Pm1ConcentrationMeasurement::Attributes::MeasuredValue::Id, data->pm1_0 },
        { Pm25ConcentrationMeasurement::Id, Pm25ConcentrationMeasurement::Attributes::MeasuredValue::Id, data->pm2_5 },
        { Pm10ConcentrationMeasurement::Id, Pm10ConcentrationMeasurement::Attributes::MeasuredValue::Id, data->pm10_0 },
        { TotalVolatileOrganicCompoundsConcentrationMeasurement::Id, TotalVolatileOrganicCompoundsConcentrationMeasurement::Attributes::MeasuredValue::Id, data->voc_index },
        { NitrogenDioxideConcentrationMeasurement::Id, NitrogenDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id, data->nox_index },
    };

    // Update int16 attributes
    for (const auto &update : int16_updates) {
        esp_matter_attr_val_t attr_val = esp_matter_int16(update.value);
        esp_err_t err = attribute::update(endpoint_id, update.cluster_id, update.attribute_id, &attr_val);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to update INT16 cluster 0x%08X attr 0x%08X (err=0x%X)",
                     static_cast<unsigned int>(update.cluster_id),
                     static_cast<unsigned int>(update.attribute_id),
                     static_cast<unsigned int>(err));
        }
    }

    // Update float attributes
    for (const auto &update : float_updates) {
        esp_matter_attr_val_t attr_val = esp_matter_float(update.value);
        esp_err_t err = attribute::update(endpoint_id, update.cluster_id, update.attribute_id, &attr_val);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to update FLOAT cluster 0x%08X attr 0x%08X (err=0x%X)",
                     static_cast<unsigned int>(update.cluster_id),
                     static_cast<unsigned int>(update.attribute_id),
                     static_cast<unsigned int>(err));
        }
    }
}


