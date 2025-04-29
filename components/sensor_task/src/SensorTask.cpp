#include "SensorTask.h"
#include <esp_log.h>
static const char *TAG = "SensorTask";

SensorTask::SensorTask(MatterAirQuality &aqCluster, uint64_t intervalUs)
  : mAqCluster(aqCluster),
    mIntervalUs(intervalUs),
    mTimer(nullptr)
{
    // prepare the esp_timer (but don’t start it yet)
    esp_timer_create_args_t args = {
        .callback = &SensorTask::timerCallback,
        .arg      = this,
        .name     = "SensorTaskTimer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &mTimer));
}

SensorTask::~SensorTask() {
    if (mTimer) {
        esp_timer_stop(mTimer);
        esp_timer_delete(mTimer);
    }
}

void SensorTask::start() {
    ESP_ERROR_CHECK(esp_timer_start_periodic(mTimer, mIntervalUs));
    ESP_LOGI(TAG, "SensorTask started @ %lluus", mIntervalUs);
}

esp_err_t SensorTask::setInterval(uint64_t intervalUs) {
    mIntervalUs = intervalUs;
    return esp_timer_stop(mTimer) == ESP_OK
        ? esp_timer_start_periodic(mTimer, mIntervalUs)
        : ESP_FAIL;
}

void SensorTask::timerCallback(void *arg) {
    static_cast<SensorTask*>(arg)->handleTimer();
}

void SensorTask::handleTimer() {
    if (!mAqCluster.ReadSensor(&mLatestData)) {
        ESP_LOGW(TAG, "SensorTask: ReadSensor failed");
        return;
    }
    mAqCluster.UpdateAirQualityAttributes(&mLatestData);
    ESP_LOGI(TAG, "Sensor read: T=%.2f RH=%.2f PM2.5=%.2f CO2=%.2f VOC=%.2f NOx=%.2f",
        mLatestData.temperature,
        mLatestData.humidity,
        mLatestData.pm2_5,
        mLatestData.co2_equivalent,
        mLatestData.voc_index,
        mLatestData.nox_index);
}