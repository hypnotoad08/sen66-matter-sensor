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
#include <app/server/CommissioningWindowManager.h>
#include <setup_payload/SetupPayload.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/ManualSetupPayloadGenerator.h>

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

// Function declarations
static void initializeNvs();
static void initializeHardware();
static node_t *createMatterNode();
static void openCommissioningWindowIfNecessary();
static void appEventCallback(const DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
static void displayMatterInfo();

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting Nano Matter Air Quality Sensor");

    // Initialize NVS
    initializeNvs();

    // Initialize hardware
    initializeHardware();

    // Create Matter node
    node_t *node = createMatterNode();
    if (!node) {
        ESP_LOGE(TAG, "Failed to create Matter node, aborting");
        abort();
    }

    // Create Air Quality object
    matterAirQuality = new MatterAirQuality(node);
    matterAirQuality->CreateAirQualityEndpoint();
    matterAirQuality->StartMeasurements();

    // Start Matter server
    ESP_ERROR_CHECK(esp_matter::start(appEventCallback));
    ESP_LOGI(TAG, "Matter server started (fabrics: %d)",
             Server::GetInstance().GetFabricTable().FabricCount());

    // Open commissioning window if necessary
    openCommissioningWindowIfNecessary();

    // Display Matter pairing information
    displayMatterInfo();

    // Synchronize time using SNTP
    if (!sntp_sync()) {
        ESP_LOGW(TAG, "SNTP synchronization failed");
    }

    // Start sensor task
    SensorTask sensorTask(*matterAirQuality);
    sensorTask.start();

    // Main loop
    while (true) {
        factory_reset_loop();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

//=============================================================================
// initializeNvs()
//=============================================================================
static void initializeNvs()
{
    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(err);
}

//=============================================================================
// initializeHardware()
//=============================================================================
static void initializeHardware()
{
    ESP_LOGI(TAG, "Initializing hardware");
    sen66_i2c_init();
    sen66_start_measurement();
    factory_reset_init();
}

//=============================================================================
// createMatterNode()
//=============================================================================
static node_t *createMatterNode()
{
    ESP_LOGI(TAG, "Creating Matter node");
    node::config_t node_cfg = {};
    return node::create(&node_cfg, /*attr_cb=*/nullptr, /*identify_cb=*/nullptr);
}

//=============================================================================
// openCommissioningWindowIfNecessary()
//=============================================================================
static void openCommissioningWindowIfNecessary()
{
    auto &mgr = Server::GetInstance().GetCommissioningWindowManager();
    if (Server::GetInstance().GetFabricTable().FabricCount() == 0 &&
        !mgr.IsCommissioningWindowOpen())
    {
        // Open a 5-minute commissioning window
        CHIP_ERROR err = mgr.OpenBasicCommissioningWindow(
            System::Clock::Seconds16(300),
            CommissioningWindowAdvertisement::kDnssdOnly);
        if (err != CHIP_NO_ERROR) {
            ESP_LOGE(TAG, "Failed to open commissioning window: %" CHIP_ERROR_FORMAT, err.Format());
        }
    }
}

//=============================================================================
// appEventCallback()
//=============================================================================
static void appEventCallback(const DeviceLayer::ChipDeviceEvent *event, intptr_t)
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
        openCommissioningWindowIfNecessary();
        break;
    default:
        break;
    }
}

//=============================================================================
// displayMatterInfo()
//=============================================================================
static void displayMatterInfo()
{
    SetupPayload payload;
    payload.version = 0;
    payload.discriminator.SetLongValue(397);
    payload.setUpPINCode = 73663224;
    payload.rendezvousInformation.SetValue(RendezvousInformationFlag::kBLE);

    std::string qr, manual;
    if (QRCodeSetupPayloadGenerator(payload).payloadBase38Representation(qr) == CHIP_NO_ERROR) {
        ESP_LOGI(TAG, "Setup QR Code: %s", qr.c_str());
    }
    if (ManualSetupPayloadGenerator(payload).payloadDecimalStringRepresentation(manual) == CHIP_NO_ERROR) {
        ESP_LOGI(TAG, "Manual pairing code: %s", manual.c_str());
    }
}
