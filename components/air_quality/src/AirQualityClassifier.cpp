#include "AirQualityClassifier.h"

AirQualityLevel AirQualityClassifier::classifyCo2(uint16_t co2_ppm) {
    for (auto& bp : CO2_THRESHOLDS) {
        if (co2_ppm <= bp.first) {
            return bp.second;
        }
    }
    return AirQualityLevel::kUnknown;
}

float AirQualityClassifier::calculateAqi(float C, const std::array<AqiBreakpoint,6>& bps) {
    if (std::isnan(C) || C < 0) {
        return NAN;
    }
    for (auto& bp : bps) {
        if (C <= bp.conc_hi) {
            float ratio = (C - bp.conc_lo) / (bp.conc_hi - bp.conc_lo);
            return ratio * (bp.aqi_hi - bp.aqi_lo) + bp.aqi_lo;
        }
    }
    return NAN;
}

AirQualityLevel AirQualityClassifier::aqiToLevel(int aqi) {
    if      (aqi <=  50) return AirQualityLevel::kGood;
    else if (aqi <= 100) return AirQualityLevel::kFair;
    else if (aqi <= 150) return AirQualityLevel::kModerate;
    else if (aqi <= 200) return AirQualityLevel::kPoor;
    else if (aqi <= 300) return AirQualityLevel::kVeryPoor;
    else if (aqi <= 500) return AirQualityLevel::kExtremelyPoor;
    else                 return AirQualityLevel::kUnknown;
}

AirQualityLevel AirQualityClassifier::classify(const sen66_data_t* d) {
    // CO₂ category
    AirQualityLevel co2Lvl  = classifyCo2(d->co2_equivalent);

    // PM indices
    float aqi25 = calculateAqi(d->pm2_5, PM25_BREAKPOINTS);
    float aqi10 = calculateAqi(d->pm10_0, PM10_BREAKPOINTS);

    // Convert to categories
    AirQualityLevel lvl25 = std::isnan(aqi25) ? AirQualityLevel::kUnknown
                                               : aqiToLevel(int(aqi25 + 0.5f));
    AirQualityLevel lvl10 = std::isnan(aqi10) ? AirQualityLevel::kUnknown
                                               : aqiToLevel(int(aqi10 + 0.5f));

    // Worst of the three
    return std::max({ co2Lvl, lvl25, lvl10 });
}