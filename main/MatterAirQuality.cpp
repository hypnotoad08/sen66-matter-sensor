#include "MatterAirQuality.h"
#include <esp_log.h>
#include <common_macros.h>

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

    {
        cluster::temperature_measurement::config_t cfg = {};
        cluster::temperature_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
    }
    {
        cluster::relative_humidity_measurement::config_t cfg = {};
        cluster::relative_humidity_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
    }
    {
        cluster::carbon_dioxide_concentration_measurement::config_t cfg = {};
        cluster::carbon_dioxide_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
    }
    {
        cluster::pm1_concentration_measurement::config_t cfg = {};
        cluster::pm1_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
    }
    {
        cluster::pm25_concentration_measurement::config_t cfg = {};
        cluster::pm25_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
    }
    {
        cluster::pm10_concentration_measurement::config_t cfg = {};
        cluster::pm10_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
    }
    {
        cluster::total_volatile_organic_compounds_concentration_measurement::config_t cfg = {};
        cluster::total_volatile_organic_compounds_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
    }
    {
        cluster::nitrogen_dioxide_concentration_measurement::config_t cfg = {};
        cluster::nitrogen_dioxide_concentration_measurement::create(m_air_quality_endpoint, &cfg, CLUSTER_FLAG_SERVER);
    }
}



void MatterAirQuality::StartMeasurements()
{
    ESP_LOGI(TAG, "MatterAirQuality started");
}

void MatterAirQuality::UpdateAirQualityAttributes(const sen66_data_t *data)
{
    struct AttributeUpdate {
        esp_matter::endpoint_t *endpoint;
        uint32_t cluster_id;
        uint32_t attribute_id;
        int16_t value;
    };

    AttributeUpdate updates[] = {
        { m_endpoint_temp, TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id, (int16_t)(data->temperature * 100) },
        { m_endpoint_hum, RelativeHumidityMeasurement::Id, RelativeHumidityMeasurement::Attributes::MeasuredValue::Id, (int16_t)(data->humidity * 100) },

        { m_endpoint_pm1, Pm1ConcentrationMeasurement::Id, Pm1ConcentrationMeasurement::Attributes::MeasuredValue::Id, (int16_t)(data->pm1_0 * 10) },
        { m_endpoint_pm25, Pm25ConcentrationMeasurement::Id, Pm25ConcentrationMeasurement::Attributes::MeasuredValue::Id, (int16_t)(data->pm2_5 * 10) },
        { m_endpoint_pm10, Pm10ConcentrationMeasurement::Id, Pm10ConcentrationMeasurement::Attributes::MeasuredValue::Id, (int16_t)(data->pm10_0 * 10) },

        { m_endpoint_co2, CarbonDioxideConcentrationMeasurement::Id, CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id, (int16_t)(data->co2_equivalent) },

        { m_endpoint_voc, TotalVolatileOrganicCompoundsConcentrationMeasurement::Id, TotalVolatileOrganicCompoundsConcentrationMeasurement::Attributes::MeasuredValue::Id, (int16_t)(data->voc_index * 10) },

        { m_endpoint_nox, NitrogenDioxideConcentrationMeasurement::Id, NitrogenDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id, (int16_t)(data->nox_index * 10) },
    };

    for (const auto &update : updates) {
        if (!update.endpoint) {
            continue;
        }

        uint16_t endpoint_id = endpoint::get_id(update.endpoint);
        cluster_t *cluster = cluster::get(endpoint_id, update.cluster_id);
        if (!cluster) {
            ESP_LOGW(TAG, "No cluster 0x%08" PRIX32 " on endpoint 0x%04X", update.cluster_id, endpoint_id);
            continue;
        }

        attribute_t *attribute = attribute::get(cluster, update.attribute_id);
        if (!attribute) {
            ESP_LOGW(TAG, "No attribute 0x%08" PRIX32 " in cluster 0x%08" PRIX32, update.attribute_id, update.cluster_id);
            continue;
        }

        esp_matter_attr_val_t attr_val = esp_matter_int16(update.value);
        esp_err_t err = attribute::update(endpoint_id, update.cluster_id, update.attribute_id, &attr_val);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Update fail 0x%x: endpoint 0x%04X cluster 0x%08" PRIX32 " attr 0x%08" PRIX32, err, endpoint_id, update.cluster_id, update.attribute_id);
        }
    }
}
