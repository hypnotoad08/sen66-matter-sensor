#include "sntp_sync.h"
#include <esp_log.h>
#include <esp_sntp.h>
#include <time.h>

static const char *TAG = "sntp";

void sntp_sync() {
    ESP_LOGI(TAG, "Initializing SNTP");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    // Wait for system time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year >= (2016 - 1900)) {
        ESP_LOGI(TAG, "Time synchronized: %s", asctime(&timeinfo));
    } else {
        ESP_LOGW(TAG, "Failed to synchronize time after %d retries", retry_count);
    }
}
