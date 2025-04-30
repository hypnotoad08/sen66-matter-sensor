#include "sntp_sync.h"
#include <esp_log.h>
#include <esp_sntp.h>
#include <time.h>

static const char *TAG = "sntp";

// Constants for retry logic
constexpr int RETRY_COUNT = 10;
constexpr int RETRY_DELAY_MS = 2000;

bool sntp_sync() {
    ESP_LOGI(TAG, "Initializing SNTP");

    // Configure SNTP
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    // Wait for system time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;

    while (timeinfo.tm_year < (2016 - 1900) && retry < RETRY_COUNT) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry + 1, RETRY_COUNT);
        vTaskDelay(RETRY_DELAY_MS / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
        retry++;
    }

    if (timeinfo.tm_year >= (2016 - 1900)) {
        ESP_LOGI(TAG, "Time synchronized: %s", asctime(&timeinfo));
        return true; // Synchronization successful
    } else {
        ESP_LOGW(TAG, "Failed to synchronize time after %d retries", RETRY_COUNT);
        return false; // Synchronization failed
    }
}
