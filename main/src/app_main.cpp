#include "MatterAirQuality.h"
#include "SensorTask.h"
#include "sen66_sensor.h"
#include "factory_reset.h"
#include "sntp_sync.h"
#include "esp_matter.h"
#include <esp_matter_cluster.h>
#include <esp_matter_attribute.h>
#include <esp_matter_providers.h>

#include <app/server/Server.h>
#include <app/server/CommissioningWindowManager.h> // Include the header for Server
#include <setup_payload/SetupPayload.h> // Include the header for SetupPayload
#include <setup_payload/QRCodeSetupPayloadGenerator.h> // Include the header for QRCodeSetupPayloadGenerator
#include <setup_payload/ManualSetupPayloadGenerator.h> // Include the header for ManualSetupPayloadGenerator

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <common_macros.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;
using namespace chip;
static const char *TAG = "main";

static MatterAirQuality *matterAirQuality = nullptr; // Global Matter Air Quality object


static void open_commissioning_window_if_necessary();
static void app_event_cb(const DeviceLayer::ChipDeviceEvent * event, intptr_t arg);
void displayMatterInfo();
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting Nano Matter Air Quality Sensor");// 1) NVS init
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(err);

    // Initialize hardware
    sen66_i2c_init();
    sen66_start_measurement();
    factory_reset_init();

    node::config_t node_cfg = {};
    node_t *node = node::create(&node_cfg, /*attr_cb=*/nullptr, /*identify_cb=*/nullptr);
if (!node) {
    ESP_LOGE(TAG, "Failed to create Matter node, aborting");
    abort();  // or infinite loop if you want
}
    ESP_LOGI(TAG, "Matter node created");


    // Create Air Quality object
    matterAirQuality = new MatterAirQuality(node);

    matterAirQuality->CreateAirQualityEndpoint();
    matterAirQuality->StartMeasurements();
    
        ESP_ERROR_CHECK(esp_matter::start(app_event_cb));
        ESP_LOGI(TAG, "Matter server started (fabrics: %d)",
            Server::GetInstance().GetFabricTable().FabricCount());
        open_commissioning_window_if_necessary();
        displayMatterInfo();
        sntp_sync();

        SensorTask sensorTask(*matterAirQuality);
        sensorTask.start();
    
        while (true) {
            factory_reset_loop();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
}

//=============================================================================
// displayMatterInfo()
//=============================================================================
void displayMatterInfo()
{
    SetupPayload payload;
    payload.version = 0;
    payload.discriminator.SetLongValue(397);
    payload.setUpPINCode = 73663224;
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
