#include "SensorTask.h"
#include <esp_log.h>
#include <cmath>
#include <SmaFilter.h>

static const char *TAG = "SensorTask";

SensorTask::SensorTask(MatterAirQuality &aqCluster, uint64_t intervalUs)
    : mAqCluster(aqCluster),
      mIntervalUs(intervalUs),
      mTimer(nullptr)

{

    // prepare the esp_timer (but don’t start it yet)
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

    sen66_data_t smooth;
    smooth.co2_equivalent = mLatestData.co2_equivalent;
    smooth.voc_index = mLatestData.voc_index;
    smooth.nox_index = mLatestData.nox_index;
    smooth.temperature = mLatestData.temperature;
    smooth.humidity = mLatestData.humidity;
    smooth.pm1_0 = pm1_filter.addSample(mLatestData.pm1_0);
    smooth.pm2_5 = pm25_filter.addSample(mLatestData.pm2_5);
    smooth.pm10_0 = pm10_filter.addSample(mLatestData.pm10_0);

    // Check thresholds:
    bool report = false;
    if (std::fabs(smooth.pm1_0 - mLastPublished.pm1_0) > kPm10Threshold)
        report = true;
    if (std::fabs(smooth.pm2_5 - mLastPublished.pm2_5) > kPm25Threshold)
        report = true;
    if (std::fabs(smooth.pm10_0 - mLastPublished.pm10_0) > kPm10Threshold)
        report = true;
    if (std::fabs(smooth.co2_equivalent - mLastPublished.co2_equivalent) > kCo2Threshold)
        report = true;
    if (std::fabs(smooth.voc_index - mLastPublished.voc_index) > kVocThreshold)
        report = true;
    if (std::fabs(smooth.nox_index - mLastPublished.nox_index) > kNoxThreshold)
        report = true;
    if (std::fabs(smooth.temperature - mLastPublished.temperature) > kTempThreshold)
        report = true;
    if (std::fabs(smooth.humidity - mLastPublished.humidity) > kHumThreshold)
        report = true;

    if (!report)
    {
        ESP_LOGD(TAG, "All changes within thresholds; skipping report");
        return;
    }

    sen66_data_t old = mLastPublished;
    mAqCluster.UpdateAirQualityAttributes(&smooth);
    mLastPublished = smooth; // Update last published data
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