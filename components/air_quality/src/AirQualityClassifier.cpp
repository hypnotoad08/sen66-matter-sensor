#include "AirQualityClassifier.h"

AirQualityLevel AirQualityClassifier::classifyCo2(uint16_t co2_ppm) {
    for (const auto& [threshold, level] : CO2_THRESHOLDS) {
        if (co2_ppm <= threshold) {
            return level;
        }
    }
    return AirQualityLevel::kUnknown;
}

float AirQualityClassifier::calculateAqi(float concentration, const std::array<AqiBreakpoint, 6>& breakpoints) {
    if (std::isnan(concentration) || concentration < 0) {
        return NAN;
    }

    for (const auto& bp : breakpoints) {
        if (concentration <= bp.conc_hi) {
            float ratio = (concentration - bp.conc_lo) / (bp.conc_hi - bp.conc_lo);
            return ratio * (bp.aqi_hi - bp.aqi_lo) + bp.aqi_lo;
        }
    }

    return NAN;
}

AirQualityLevel AirQualityClassifier::aqiToLevel(int aqi) {
    if (aqi <= 50) return AirQualityLevel::kGood;
    if (aqi <= 100) return AirQualityLevel::kFair;
    if (aqi <= 150) return AirQualityLevel::kModerate;
    if (aqi <= 200) return AirQualityLevel::kPoor;
    if (aqi <= 300) return AirQualityLevel::kVeryPoor;
    if (aqi <= 500) return AirQualityLevel::kExtremelyPoor;
    return AirQualityLevel::kUnknown;
}

AirQualityLevel AirQualityClassifier::classify(const sen66_data_t* data) {
    if (!data) {
        return AirQualityLevel::kUnknown;
    }

    // Classify CO₂
    AirQualityLevel co2Level = classifyCo2(data->co2_equivalent);

    // Calculate AQI for PM2.5 and PM10
    float aqi25 = calculateAqi(data->pm2_5, PM25_BREAKPOINTS);
    float aqi10 = calculateAqi(data->pm10_0, PM10_BREAKPOINTS);

    // Convert AQI to levels
    AirQualityLevel pm25Level = std::isnan(aqi25) ? AirQualityLevel::kUnknown : aqiToLevel(static_cast<int>(aqi25 + 0.5f));
    AirQualityLevel pm10Level = std::isnan(aqi10) ? AirQualityLevel::kUnknown : aqiToLevel(static_cast<int>(aqi10 + 0.5f));

    // Return the worst level among CO₂, PM2.5, and PM10
    return std::max({ co2Level, pm25Level, pm10Level });
}