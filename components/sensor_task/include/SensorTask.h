#pragma once
#include <esp_timer.h>
#include "MatterAirQuality.h"

class SensorTask {
public:
    SensorTask(MatterAirQuality &aqCluster, uint64_t intervalUs = 5ULL * 1000 * 1000);
    ~SensorTask();

    // start the periodic reads
    void start();

    // optionally change interval at runtime
    esp_err_t setInterval(uint64_t intervalUs);

private:
    static void timerCallback(void *arg);
    void handleTimer();

    MatterAirQuality &mAqCluster;
    uint64_t          mIntervalUs;
    esp_timer_handle_t mTimer;
    sen66_data_t      mLatestData;
    sen66_data_t      mLastPublished{};
    const float kPm10Threshold   = 1.0f;   // µg/m³
    const float kPm25Threshold   = 1.0f;   // µg/m³
    const float kCo2Threshold    = 50.0f;  // ppm
    const float kVocThreshold    = 10.0f;  // ppb
    const float kNoxThreshold    = 5.0f;   // ppb
    const float kTempThreshold   = 0.5f;   // °C
    const float kHumThreshold    = 2.0f;   // %
};