#pragma once

#include <array>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include "MatterAirQuality.h"  // for AirQualityLevel and sen66_data_t

// Enum representing air quality levels
enum AirQualityLevel : int16_t {
    kGood            = 0,
    kFair            = 1,
    kModerate        = 2,
    kPoor            = 3,
    kVeryPoor        = 4,
    kExtremelyPoor   = 5,
    kUnknown         = 6
};

// Struct representing AQI breakpoints for linear interpolation
struct AqiBreakpoint {
    float conc_lo, conc_hi; // Concentration range
    int   aqi_lo,   aqi_hi; // AQI range
};

class AirQualityClassifier {
public:
    // Map CO₂ concentration to AirQualityLevel via thresholds
    static AirQualityLevel classifyCo2(uint16_t co2_ppm);

    // Calculate AQI index using linear interpolation
    static float calculateAqi(float concentration, const std::array<AqiBreakpoint, 6>& breakpoints);

    // Map AQI index (0-500) to AirQualityLevel categories
    static AirQualityLevel aqiToLevel(int aqi);

    // Master classification combining CO₂, PM2.5, and PM10
    static AirQualityLevel classify(const sen66_data_t* data);

private:
    // Breakpoints for PM2.5 and PM10
    static constexpr std::array<AqiBreakpoint, 6> PM25_BREAKPOINTS = {{
        { 0.0f, 12.0f, 0, 50 },
        { 12.1f, 35.4f, 51, 100 },
        { 35.5f, 55.4f, 101, 150 },
        { 55.5f, 150.4f, 151, 200 },
        { 150.5f, 250.4f, 201, 300 },
        { 250.5f, FLT_MAX, 301, 500 }
    }};

    static constexpr std::array<AqiBreakpoint, 6> PM10_BREAKPOINTS = {{
        { 0.0f, 54.0f, 0, 50 },
        { 55.0f, 154.0f, 51, 100 },
        { 155.0f, 254.0f, 101, 150 },
        { 255.0f, 354.0f, 151, 200 },
        { 355.0f, 424.0f, 201, 300 },
        { 425.0f, FLT_MAX, 301, 500 }
    }};

    // Thresholds for CO₂ classification
    static constexpr std::array<std::pair<uint16_t, AirQualityLevel>, 6> CO2_THRESHOLDS = {{
        { 600, AirQualityLevel::kGood },
        { 700, AirQualityLevel::kFair },
        { 800, AirQualityLevel::kModerate },
        { 950, AirQualityLevel::kPoor },
        { 1200, AirQualityLevel::kVeryPoor },
        { UINT16_MAX, AirQualityLevel::kExtremelyPoor }
    }};
};
