#pragma once

#include <cstdint>
#include <optional>

// Raw values read from game memory.
struct GameSnapshot {
    std::optional<uint32_t> p1Character;
    std::optional<uint32_t> p2Character;
    std::optional<uint32_t> p1TagCharacter;
    std::optional<uint32_t> p2TagCharacter;
    std::optional<uint32_t> p1PickStep;
    std::optional<uint32_t> p2PickStep;
    std::optional<uint32_t> p1ActiveCharacter;
    std::optional<uint32_t> p2ActiveCharacter;
    std::optional<uint32_t> p1Controller;
    std::optional<uint32_t> p2Controller;
    std::optional<uint32_t> stage;
    std::optional<uint32_t> localPlayerSide;
    std::optional<uint32_t> gameMode;
    std::optional<uint32_t> inMatch;
    std::optional<uint32_t> difficulty;
    std::optional<uint32_t> modeVariant;
    std::optional<uint32_t> replay;
    std::optional<uint32_t> arcadeStage;
    std::optional<uint32_t> uiLayer;
    std::optional<uint32_t> spectating;
    std::optional<uint32_t> lobbySlots;
    std::optional<uint32_t> playerLobby;
    std::optional<uint32_t> lobbyMembers; // counted from the lobby connection table

    bool operator==(const GameSnapshot& o) const;
    bool operator!=(const GameSnapshot& o) const { return !(*this == o); }
};

GameSnapshot readGame();
