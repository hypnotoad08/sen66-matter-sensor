#pragma once

namespace sen66_ranges {

// particulate matter (all three channels share the same span)
static constexpr float PM_MIN   = 0.0f;
static constexpr float PM_MAX   = 1000.0f;

// gas indices & equivalents
static constexpr float ECO2_MIN = 0.0f;
static constexpr float ECO2_MAX = 8192.0f;
static constexpr float VOC_MIN  = 0.0f;
static constexpr float VOC_MAX  = 500.0f;
static constexpr float NOX_MIN  = 0.0f;
static constexpr float NOX_MAX  = 500.0f;

// environmental
static constexpr float TEMP_MIN = -40.0f;
static constexpr float TEMP_MAX = 85.0f;
static constexpr float HUM_MIN  = 0.0f;
static constexpr float HUM_MAX  = 100.0f;

}