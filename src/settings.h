#pragma once

#include <cstdint>

namespace settings {
    
// Plugin version, shown in the large image's hover text and the log.
constexpr const char* kVersion = "1.4.0";

// Discord application ID.
constexpr int64_t kClientId = 1564444889482264639;
    
// Default icon for each config Cover option.
constexpr const char* kDefaultLargeImage = "logo";
constexpr const char* kDefaultJpLargeImage = "logo_jp";
constexpr const char* kUltimateLargeImage = "logo_grid";
constexpr const char* kUltimateJpLargeImage = "logo_ultimate";
constexpr const char* kVanillaLargeImage = "logo_original";
    
// Update rate in matches.
constexpr unsigned kUpdateIntervalMs = 500;

// Write DOA5Discord.log next to the .asi with every value change.
constexpr bool kWriteLog = true;

}
