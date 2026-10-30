#pragma once

#include <cstdint>
#include <optional>

// Static addresses collected within game.exe (32-bit), as offsets from the module base.
namespace addr {

// Selected Character IDs per slot.
constexpr uint32_t kP1Character = 0x2053E82;
constexpr uint32_t kP2Character = 0x2053ED2;

// Each team's character currently on screen.
constexpr uint32_t kP1ActiveCharacter = 0xFCED80;
constexpr uint32_t kP2ActiveCharacter = 0xFCEDB4;

// Current stage.
constexpr uint32_t kStage = 0xDA442C;

// 0 = the local player is P1, 1 = P2. Reads 0xFF between fights.
constexpr uint32_t kLocalPlayerSide = 0x10877DC;

// Game mode ID. Only updates when a fight starts, so it keeps the last mode in the menus.
constexpr uint32_t kGameMode = 0xFCDAD6;

// 1 while a fight is running, 0 in the menus and between fights.
constexpr uint32_t kInMatch = 0xF8A977;

// 1 while a replay plays.
constexpr uint32_t kReplay = 0x2054697;

// Address of the given mode's 0-based difficulty, if it has one.
std::optional<uint32_t> difficultyFor(uint32_t mode);

// Address of a byte that splits the given mode further (e.g. Free vs Command Training).
std::optional<uint32_t> variantFor(uint32_t mode);

// Unused for now:
//   0x2053F22 / 0x2053F72  Team 1 / team 2 tag partners (Player 2 & Player 4)
//   0xFCDAAE               Tag battle flag; also 1 in Team Fight
//   0x20544A8              Cheat table "Stage A"; can hold the previous stage
//   0x10877B8              Controller index per side (0xFF = CPU)
//   0xFCDAD8               Arcade stage number from 0; also counts Survival fights; 0 outside a run
//   0xF98314               Arcade/Time Attack stage number from 1; keeps its value after a run
//   0xD2EBB8               Backup replay flag

}
