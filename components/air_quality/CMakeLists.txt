# components/air_quality/CMakeLists.txt
idf_component_register(
    SRCS
        src/MatterAirQuality.cpp
    INCLUDE_DIRS
        include
        ${ESP_MATTER_PATH}/examples/common/utils
    REQUIRES
        esp_matter
        sen66
)
set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)

