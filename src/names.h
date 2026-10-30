#pragma once

#include <cstdint>
#include <optional>
#include <string>

constexpr uint32_t kNoCharacter = 0xFF;

// Display names, or nullptr for an unknown ID.
const char* characterName(uint32_t id);
const char* stageName(uint32_t id);

// Returns the game mode name.
const char* gameModeName(uint32_t mode, std::optional<uint32_t> variant);

// Returns the difficulty name based on the level.
const char* difficultyName(uint32_t level);

// Modes where nobody is playing (Watch Mode): show "P1 vs P2" and no character image.
bool isSpectatorMode(uint32_t mode);

// Discord asset key of the stage's image, or nullptr if no image was made for it.
const char* stageImage(uint32_t stage);

// Returns the discord asset key for a display name.
std::string assetKey(const std::string& name);
