#pragma once
#include <esp_timer.h>
#include "MatterAirQuality.h"
#include <SmaFilter.h>

class SensorTask
{
public:
    SensorTask(MatterAirQuality &aqCluster, uint64_t intervalUs = 5ULL * 1000 * 1000);
    ~SensorTask();

    // Start the periodic sensor reads
    void start();

    // Change the interval at runtime
    esp_err_t setInterval(uint64_t intervalUs);

private:
    // Timer callback and handler
    static void timerCallback(void *arg);
    void handleTimer();

    // Helper methods
    void smoothSensorData(sen66_data_t &smooth);
    bool shouldReport(const sen66_data_t &smooth) const;
    void logChanges(const sen66_data_t &smooth, const sen66_data_t &old) const;
    void saveLastPublishedToNVS() const;

    // Member variables
    MatterAirQuality &mAqCluster;
    uint64_t mIntervalUs;
    esp_timer_handle_t mTimer;
    sen66_data_t mLatestData;
    sen66_data_t mLastPublished{};

    // Thresholds for reporting
    static constexpr float kPm10Threshold = 1.0f; // µg/m³
    static constexpr float kPm25Threshold = 1.0f; // µg/m³
    static constexpr float kCo2Threshold = 50.0f; // ppm
    static constexpr float kVocThreshold = 10.0f; // ppb
    static constexpr float kNoxThreshold = 5.0f;  // ppb
    static constexpr float kTempThreshold = 0.5f; // °C
    static constexpr float kHumThreshold = 2.0f;  // %

    // Filters for smoothing data
    SmaFilter pm1_filter{5}, pm25_filter{5}, pm10_filter{5};
};