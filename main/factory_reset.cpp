#include "factory_reset.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "app_reset.h"
#include <esp_timer.h>
#include <esp_matter.h> // Include the header file for esp_matter

static const char *TAG = "factory_reset";

#define RESET_BUTTON_GPIO GPIO_NUM_10
#define RESET_LED_GPIO    GPIO_NUM_21
#define HOLD_TIME_MS      5000

static uint32_t hold_time = 0;
static bool led_state = false;

static bool commissioning_active = false;
static bool connected_to_network = false;

void factory_reset_set_commissioning(bool commissioning) {
    commissioning_active = commissioning;
}

void factory_reset_set_connected(bool connected) {
    connected_to_network = connected;
}

void factory_reset_init() {
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << RESET_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&btn_conf);

    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << RESET_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&led_conf);

    gpio_set_level(RESET_LED_GPIO, 0);
}

void factory_reset_loop() {
    static uint32_t last_blink = 0;
    uint32_t now = esp_timer_get_time() / 1000;
    int pressed = (gpio_get_level(RESET_BUTTON_GPIO) == 0);

    if (pressed) {
        hold_time += 100;
        uint32_t blink_interval = (hold_time < HOLD_TIME_MS) ? 500 : 100;

        if (now - last_blink > blink_interval) {
            led_state = !led_state;
            gpio_set_level(RESET_LED_GPIO, led_state);
            last_blink = now;
        }

        if (hold_time >= HOLD_TIME_MS + 1000) {
            ESP_LOGI(TAG, "Factory reset triggered");
            esp_matter::factory_reset();
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
    } else if (commissioning_active) {
        uint32_t blink_interval = 500;
        if (now - last_blink > blink_interval) {
            led_state = !led_state;
            gpio_set_level(RESET_LED_GPIO, led_state);
            last_blink = now;
        }
    } else if (connected_to_network) {
        gpio_set_level(RESET_LED_GPIO, 1);
    } else {
        gpio_set_level(RESET_LED_GPIO, 0);
    }

    if (!pressed) {
        hold_time = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}
