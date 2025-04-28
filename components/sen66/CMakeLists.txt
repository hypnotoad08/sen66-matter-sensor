# components/sen66/CMakeLists.txt
idf_component_register(
    SRCS
        src/sen66_i2c.c
        src/sen66_sensor.cpp
        src/sensirion_common.c
        src/sensirion_i2c.c
        src/sensirion_i2c_hal.c
    INCLUDE_DIRS
        include
    REQUIRES
        driver
)
