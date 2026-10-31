#pragma once

#include <cstdint>
#include <optional>

// Static addresses collected within game.exe (32-bit), as offsets from the module base.
namespace addr {

// Selected Character IDs per slot. On character select they follow the cursor of the side
// that's picking.
constexpr uint32_t kP1Character = 0x2053E82;
constexpr uint32_t kP2Character = 0x2053ED2;

// Tag partner slots (cheat table Player 2 / Player 4). On character select they follow the
// cursor while that side picks its partner.
constexpr uint32_t kP1TagCharacter = 0x2053F22;
constexpr uint32_t kP2TagCharacter = 0x2053F72;

// Character select pick step per side: 0 = picking the first character, 1 = picking the tag
// partner, 0xFF when that side isn't picking. Both sides can be picking at once.
constexpr uint32_t kP1PickStep = 0xFBB170;
constexpr uint32_t kP2PickStep = 0xFBC320;

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

// Arcade stage number from 0 (Tag Arcade too). Also counts Survival fights; 0 outside a run.
constexpr uint32_t kArcadeStage = 0xFCDAD8;

// Current UI layer (4-byte int). In menus it's the screen: 9 = main menu, 10 = main menu
// submenu, 4 = character select, 5 = stage select, 14 = results. -1 while a fight loads,
// then 0 once it's playing (11, 55, 71 are in-fight overlays such as Training's menu or replay controls).
constexpr uint32_t kUiLayer = 0xF8A7AC;

// Values of kUiLayer.
namespace ui {
constexpr uint32_t kLoading = 0xFFFFFFFF;
constexpr uint32_t kCharacterSelect = 4;
constexpr uint32_t kStageSelect = 5;
}

// Address of the given mode's 0-based difficulty, if it has one.
std::optional<uint32_t> difficultyFor(uint32_t mode);

// Address of a byte that splits the given mode further (e.g. Free vs Command Training).
std::optional<uint32_t> variantFor(uint32_t mode);

// Unused for now:
//   0xFBB169               Which side is picking on character select (0 = P1, 1 = P2)
//   0xFBA295               1 while P1 picks its tag partner, 0 otherwise
//   0xFCDAAE               Tag battle flag; also 1 in Team Fight
//   0x20544A8              Cheat table "Stage A"; can hold the previous stage
//   0x10877B8              Controller index per side (0xFF = CPU)
//   0xF98314               Arcade/Time Attack stage number from 1; keeps its value after a run
//   0xD2EBB8               Backup replay flag
//   0xF8A758               Menu screen (4 bytes); like kUiLayer but doesn't follow online modes

}
