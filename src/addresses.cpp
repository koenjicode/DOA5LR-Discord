#include "addresses.h"

#include <unordered_map>

namespace addr {

namespace {

// Difficulty used for each game mode (Tag has the same levels offset from 8)
const std::unordered_map<uint32_t, uint32_t> kDifficultyByMode = {
    {0x02, 0xF8A8B0}, // Arcade
    {0x03, 0xF8A8B4}, // Time Attack
    {0x04, 0xF8A8B8}, // Survival
};

const std::unordered_map<uint32_t, uint32_t> kVariantByMode = {
    {0x02, 0xFCDAAE}, // Arcade: 0 = solo, 1 = Tag Arcade
    {0x05, 0xD3989B}, // Training: 0 = Free Training, 1 = Command Training
};

std::optional<uint32_t> lookup(const std::unordered_map<uint32_t, uint32_t>& table, uint32_t mode) {
    auto it = table.find(mode);
    if (it == table.end()) return std::nullopt;
    return it->second;
}

} // namespace

std::optional<uint32_t> difficultyFor(uint32_t mode) { return lookup(kDifficultyByMode, mode); }
std::optional<uint32_t> variantFor(uint32_t mode) { return lookup(kVariantByMode, mode); }

} // namespace addr
