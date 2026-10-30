#pragma once

#include <cstdint>

namespace settings {
    
// Discord application ID.
constexpr int64_t kClientId = 1564444889482264639;
    
// Default icon to be used.   
constexpr const char* kDefaultLargeImage = "logo";
    
// Update rate in matches.
constexpr unsigned kUpdateIntervalMs = 500;

// Write DOA5Discord.log next to the .asi with every value change.
constexpr bool kWriteLog = false;

}
