#include "game.h"
#include "addresses.h"

#include <Windows.h>
#include <cstring>

namespace {

// Copies memory without crashing the game if the page is gone or protected.
bool safeRead(uintptr_t address, void* out, size_t size) {
    __try {
        memcpy(out, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Reads the byte at game.exe + offset.
std::optional<uint32_t> read(uint32_t offset) {
    static const uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    uint8_t value = 0;
    if (!safeRead(base + offset, &value, sizeof(value))) return std::nullopt;
    return value;
}

} // namespace

bool GameSnapshot::operator==(const GameSnapshot& o) const {
    return p1Character == o.p1Character && p2Character == o.p2Character &&
           p1ActiveCharacter == o.p1ActiveCharacter && p2ActiveCharacter == o.p2ActiveCharacter &&
           stage == o.stage && localPlayerSide == o.localPlayerSide && gameMode == o.gameMode &&
           inMatch == o.inMatch && difficulty == o.difficulty && modeVariant == o.modeVariant &&
           replay == o.replay;
}

GameSnapshot readGame() {
    GameSnapshot s;
    s.p1Character = read(addr::kP1Character);
    s.p2Character = read(addr::kP2Character);
    s.p1ActiveCharacter = read(addr::kP1ActiveCharacter);
    s.p2ActiveCharacter = read(addr::kP2ActiveCharacter);
    s.stage = read(addr::kStage);
    s.localPlayerSide = read(addr::kLocalPlayerSide);
    s.gameMode = read(addr::kGameMode);
    s.inMatch = read(addr::kInMatch);
    s.replay = read(addr::kReplay);
    if (s.gameMode) {
        if (auto difficulty = addr::difficultyFor(*s.gameMode)) s.difficulty = read(*difficulty);
        if (auto variant = addr::variantFor(*s.gameMode)) s.modeVariant = read(*variant);
    }
    return s;
}
