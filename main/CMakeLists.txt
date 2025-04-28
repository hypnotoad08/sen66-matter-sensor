idf_component_register(
  SRCS
    src/app_main.cpp
    src/factory_reset.cpp
    src/sntp_sync.cpp

  INCLUDE_DIRS
    src
    "${ESP_MATTER_PATH}/examples/common/utils" 
    "${MATTER_SDK_PATH}/src"                    

  REQUIRES
    air_quality
    nvs_flash
    esp_timer
    esp_matter
)

if (CONFIG_ENABLE_SET_CERT_DECLARATION_API)
  target_add_binary_data(${COMPONENT_TARGET}
    "src/certification_declaration/certification_declaration.der" BINARY
  )
endif()

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")


