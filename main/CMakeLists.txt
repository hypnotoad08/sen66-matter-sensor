idf_component_register(
    SRCS
    "app_main.cpp"
    "sen66_sensor.cpp"
    "factory_reset.cpp"
    "MatterAirQuality.cpp"
    "sntp_sync.cpp"
    "sen66_i2c.c"
    "sensirion_common.c"
    "sensirion_i2c.c"
    "sensirion_i2c_hal.c"
    "."
    PRIV_INCLUDE_DIRS
        .
        "${ESP_MATTER_PATH}/examples/common/utils"
        "${MATTER_SDK_PATH}/src"
    PRIV_REQUIRES
        esp_matter
        esp_timer 
        nvs_flash
)

if (CONFIG_ENABLE_SET_CERT_DECLARATION_API)
    target_add_binary_data(${COMPONENT_TARGET} "certification_declaration/certification_declaration.der" BINARY)
endif()

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
