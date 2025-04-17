#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include <esp_matter.h>
#include <esp_matter_cluster.h>
#include <esp_matter_attribute.h>
#include <esp_matter_providers.h>

#include <app/server/Server.h>
#include <app/server/CommissioningWindowManager.h>
#include <lib/support/CodeUtils.h>

#include <setup_payload/SetupPayload.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/ManualSetupPayloadGenerator.h>

#include <platform/CHIPDeviceLayer.h>
#include <platform/DeviceInfoProvider.h>

#include "common_macros.h"
#include "app_reset.h"
#include "driver/temperature_sensor.h"

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;
using namespace chip;

static const char *TAG = "app_main";

// UART definitions
constexpr uart_port_t UART_PORT_NUM = UART_NUM_1;
constexpr int UART_TX_PIN           = 43;
constexpr int UART_RX_PIN           = 44;
constexpr int UART_BUF_SIZE         = 1024;
constexpr int UART_BAUD_RATE        = 115200;

// Reset button definitions
#define RESET_BUTTON_GPIO       static_cast<gpio_num_t>(0)
#define RESET_PRESS_TIME_SEC    5

// Forward declarations
static void open_commissioning_window_if_necessary();
static void app_event_cb(const DeviceLayer::ChipDeviceEvent * event, intptr_t arg);
static void reset_button_task(void * pvParameters);
void displayMatterInfo();
void uart_init();
void uart_rx_task(void * arg);

//=============================================================================
// app_main()
//=============================================================================
extern "C" void app_main()
{
    // 1) NVS init
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(err);

    // 2) Factory‑reset button
    xTaskCreate(reset_button_task, "reset_task", 4096, nullptr, 5, nullptr);

    // 3) Logging & UART
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "Starting Matter Application");
    uart_init();
    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, nullptr, 10, nullptr);

    // 4) Create Matter node (no attribute/identify callbacks for now)
    node::config_t node_cfg = {};
    node_t * node = node::create(&node_cfg, /*attr_cb=*/nullptr, /*identify_cb=*/nullptr);
    ABORT_APP_ON_FAILURE(node, ESP_LOGE(TAG, "Failed to create Matter node"));
    ESP_LOGI(TAG, "Matter node created");

    // 5) Start server & open commissioning window
    ESP_ERROR_CHECK(esp_matter::start(app_event_cb));
    ESP_LOGI(TAG, "Matter server started (fabrics: %d)",
             Server::GetInstance().GetFabricTable().FabricCount());
    open_commissioning_window_if_necessary();
    displayMatterInfo();

    // 6) Register endpoints
    temperature_sensor::config_t temp_cfg = {};
    endpoint_t * temp_ep = temperature_sensor::create(node, &temp_cfg, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(temp_ep, ESP_LOGE(TAG, "Failed to create Temperature sensor"));

    humidity_sensor::config_t hum_cfg = {};
    endpoint_t * hum_ep = humidity_sensor::create(node, &hum_cfg, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(hum_ep, ESP_LOGE(TAG, "Failed to create Humidity sensor"));

    // 7) Stub sensor updates every 10 s
    struct Endpoints { endpoint_t * temp, *hum; };
    static Endpoints eps{ temp_ep, hum_ep };

    xTaskCreate(
        [](void * param) {
            auto * e = static_cast<Endpoints *>(param);
            while (true) {
                vTaskDelay(pdMS_TO_TICKS(10000));

                DeviceLayer::SystemLayer().ScheduleLambda([=]() {
                    // Temperature = 22.5 °C
                    esp_matter_attr_val_t tv;
                    tv.val.i16 = int16_t(22.5f * 100);
                    attribute::update(
                      endpoint::get_id(e->temp),
                      TemperatureMeasurement::Id,
                      TemperatureMeasurement::Attributes::MeasuredValue::Id,
                      &tv
                    );
                    // Humidity = 55%
                    esp_matter_attr_val_t hv;
                    hv.val.u16 = uint16_t(55.0f * 100);
                    attribute::update(
                      endpoint::get_id(e->hum),
                      RelativeHumidityMeasurement::Id,
                      RelativeHumidityMeasurement::Attributes::MeasuredValue::Id,
                      &hv
                    );
                });
            }
        },
        "stub_task", 4096, &eps, 5, nullptr
    );

    ESP_LOGI(TAG, "app_main complete; scheduler running");
}

//=============================================================================
// uart_init()
//=============================================================================
void uart_init()
{
    uart_config_t cfg = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk= UART_SCLK_DEFAULT
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN,
                                UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    const char * msg = "UART Initialized.\r\n";
    uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
}

//=============================================================================
// uart_rx_task()
//=============================================================================
void uart_rx_task(void * arg)
{
    uint8_t buf[UART_BUF_SIZE];
    while (true) {
        int len = uart_read_bytes(UART_PORT_NUM, buf, sizeof(buf) - 1, pdMS_TO_TICKS(1000));
        if (len > 0) {
            buf[len] = '\0';
            ESP_LOGI(TAG, "UART RX: %s", buf);
        }
    }
}

//=============================================================================
// displayMatterInfo()
//=============================================================================
void displayMatterInfo()
{
    SetupPayload payload;
    payload.version = 0;
    payload.discriminator.SetLongValue(2516);
    payload.setUpPINCode = 19515139;
    payload.rendezvousInformation.SetValue(RendezvousInformationFlag::kBLE);

    std::string qr, manual;
    if (QRCodeSetupPayloadGenerator(payload)
          .payloadBase38Representation(qr) == CHIP_NO_ERROR)
    {
        ESP_LOGI(TAG, "Setup QR Code: %s", qr.c_str());
    }
    if (ManualSetupPayloadGenerator(payload)
          .payloadDecimalStringRepresentation(manual) == CHIP_NO_ERROR)
    {
        ESP_LOGI(TAG, "Manual pairing code: %s", manual.c_str());
    }
}

//=============================================================================
// open_commissioning_window_if_necessary()
//=============================================================================
static void open_commissioning_window_if_necessary()
{
    auto & mgr = Server::GetInstance().GetCommissioningWindowManager();
    if (Server::GetInstance().GetFabricTable().FabricCount() == 0 &&
        !mgr.IsCommissioningWindowOpen())
    {
        // 5 min, advertise DNS‑SD + BLE
        CHIP_ERROR err = mgr.OpenBasicCommissioningWindow(
            System::Clock::Seconds16(300),
            CommissioningWindowAdvertisement::kDnssdOnly
        );
        if (err != CHIP_NO_ERROR) {
            ESP_LOGE(TAG, "OpenCommWindow failed: %" CHIP_ERROR_FORMAT, err.Format());
        }
    }
}

//=============================================================================
// app_event_cb()
//=============================================================================
static void app_event_cb(const DeviceLayer::ChipDeviceEvent * event, intptr_t)
{
    switch (event->Type) {
    case DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "IP address changed");
        break;
    case DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        break;
    case DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;
    case DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Failsafe timer expired (commissioning failed)");
        break;
    case DeviceLayer::DeviceEventType::kFabricRemoved:
        ESP_LOGW(TAG, "Fabric removed, reopening window");
        open_commissioning_window_if_necessary();
        break;
    default:
        break;
    }
}

//=============================================================================
// reset_button_task()
//=============================================================================
static void reset_button_task(void * pv)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << RESET_BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io);

    bool held     = false;
    TickType_t ts = 0;

    while (true) {
        if (gpio_get_level(RESET_BUTTON_GPIO) == 0) {
            if (!held) {
                held = true;
                ts   = xTaskGetTickCount();
            } else if (xTaskGetTickCount() - ts >= pdMS_TO_TICKS(RESET_PRESS_TIME_SEC * 1000)) {
                ESP_LOGW(TAG, "Factory reset (hold %d s)", RESET_PRESS_TIME_SEC);
                DeviceLayer::PlatformMgr().ScheduleWork(
                    [](intptr_t) {
                        Server::GetInstance().ScheduleFactoryReset();
                    }, 0);
                vTaskDelay(pdMS_TO_TICKS(1000));  // debounce
            }
        } else {
            held = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
