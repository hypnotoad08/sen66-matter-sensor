#include "app_reset.h"
#include <esp_matter_core.h>

void app_reset_factory_reset() {
    esp_matter::factory_reset();
}
