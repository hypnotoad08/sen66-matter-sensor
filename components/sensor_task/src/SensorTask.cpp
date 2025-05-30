#include "SensorTask.h"
#include <esp_log.h>
#include <cmath>
#include <SmaFilter.h>
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "SensorTask";

static const char *NVS_NAMESPACE = "aq_task";
static const char *NVS_KEY_LAST = "lastValues";

SensorTask::SensorTask(MatterAirQuality &aqCluster, uint64_t intervalUs)
    : mAqCluster(aqCluster),
      mIntervalUs(intervalUs),
      mTimer(nullptr)
{

    // Open our namespace
    nvs_handle handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK)
    {
        size_t required = sizeof(mLastPublished);
        // Try to read the blob of the same size as our struct
        if (nvs_get_blob(handle, NVS_KEY_LAST, &mLastPublished, &required) != ESP_OK)
        {
            // No saved data yet — start fresh at zero
            memset(&mLastPublished, 0, sizeof(mLastPublished));
        }
        nvs_close(handle);
    }

    // Prepare the esp_timer (but don’t start it yet)
    esp_timer_create_args_t args = {
        .callback = &SensorTask::timerCallback,
        .arg = this,
        .name = "SensorTaskTimer"};
    ESP_ERROR_CHECK(esp_timer_create(&args, &mTimer));
}

SensorTask::~SensorTask()
{
    if (mTimer)
    {
        esp_timer_stop(mTimer);
        esp_timer_delete(mTimer);
    }
}

void SensorTask::start()
{
    ESP_ERROR_CHECK(esp_timer_start_periodic(mTimer, mIntervalUs));
    ESP_LOGI(TAG, "SensorTask started @ %lluus", mIntervalUs);
}

esp_err_t SensorTask::setInterval(uint64_t intervalUs)
{
    mIntervalUs = intervalUs;
    return esp_timer_stop(mTimer) == ESP_OK
               ? esp_timer_start_periodic(mTimer, mIntervalUs)
               : ESP_FAIL;
}

void SensorTask::timerCallback(void *arg)
{
    static_cast<SensorTask *>(arg)->handleTimer();
}

void SensorTask::handleTimer()
{
    if (!mAqCluster.ReadSensor(&mLatestData))
    {
        ESP_LOGW(TAG, "SensorTask: ReadSensor failed");
        return;
    }

    if (!std::isfinite(mLatestData.pm1_0) ||
        !std::isfinite(mLatestData.pm2_5) ||
        !std::isfinite(mLatestData.pm10_0) ||
        !std::isfinite(mLatestData.co2_equivalent) ||
        !std::isfinite(mLatestData.voc_index) ||
        !std::isfinite(mLatestData.nox_index) ||
        !std::isfinite(mLatestData.temperature) ||
        !std::isfinite(mLatestData.humidity))
    {
        ESP_LOGW(TAG, "SensorTask: Invalid reading(s) detected, skipping this cycle");
        return;
    }

    sen66_data_t smooth;
    smoothSensorData(smooth);

    if (!shouldReport(smooth))
    {
        ESP_LOGD(TAG, "All changes within thresholds; skipping report");
        return;
    }

    logChanges(smooth, mLastPublished);
    mAqCluster.UpdateAirQualityAttributes(&smooth);
    mLastPublished = smooth; // Update last published data
    saveLastPublishedToNVS();
}

void SensorTask::smoothSensorData(sen66_data_t &smooth)
{
    smooth.co2_equivalent = mLatestData.co2_equivalent;
    smooth.voc_index = mLatestData.voc_index;
    smooth.nox_index = mLatestData.nox_index;
    smooth.temperature = mLatestData.temperature;
    smooth.humidity = mLatestData.humidity;
    smooth.pm1_0 = pm1_filter.addSample(mLatestData.pm1_0);
    smooth.pm2_5 = pm25_filter.addSample(mLatestData.pm2_5);
    smooth.pm10_0 = pm10_filter.addSample(mLatestData.pm10_0);
}

bool SensorTask::shouldReport(const sen66_data_t &smooth) const
{
    return std::fabs(smooth.pm1_0 - mLastPublished.pm1_0) > kPm10Threshold ||
           std::fabs(smooth.pm2_5 - mLastPublished.pm2_5) > kPm25Threshold ||
           std::fabs(smooth.pm10_0 - mLastPublished.pm10_0) > kPm10Threshold ||
           std::fabs(smooth.co2_equivalent - mLastPublished.co2_equivalent) > kCo2Threshold ||
           std::fabs(smooth.voc_index - mLastPublished.voc_index) > kVocThreshold ||
           std::fabs(smooth.nox_index - mLastPublished.nox_index) > kNoxThreshold ||
           std::fabs(smooth.temperature - mLastPublished.temperature) > kTempThreshold ||
           std::fabs(smooth.humidity - mLastPublished.humidity) > kHumThreshold;
}

void SensorTask::logChanges(const sen66_data_t &smooth, const sen66_data_t &old) const
{
    ESP_LOGI(TAG,
             "Published: ΔPM1=%.1f ΔPM2.5=%.1f ΔPM10=%.1f ΔCO2=%.0f ΔVOC=%.0f ΔNOx=%.0f ΔT=%.1f ΔRH=%.1f",
             smooth.pm1_0 - old.pm1_0,
             smooth.pm2_5 - old.pm2_5,
             smooth.pm10_0 - old.pm10_0,
             smooth.co2_equivalent - old.co2_equivalent,
             smooth.voc_index - old.voc_index,
             smooth.nox_index - old.nox_index,
             smooth.temperature - old.temperature,
             smooth.humidity - old.humidity);
}

void SensorTask::saveLastPublishedToNVS() const
{
    nvs_handle handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to open NVS namespace");
        return;
    }
    // Write the whole struct as a blob
    esp_err_t err = nvs_set_blob(handle, NVS_KEY_LAST,
                                 &mLastPublished,
                                 sizeof(mLastPublished));
    if (err == ESP_OK)
    {
        nvs_commit(handle);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to write lastPublished to NVS (%d)", err);
    }
    nvs_close(handle);
}