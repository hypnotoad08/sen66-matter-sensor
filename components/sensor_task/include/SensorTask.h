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
};