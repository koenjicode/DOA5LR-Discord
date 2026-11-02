#pragma once

#include <cstdint>
#include <string>

struct Presence {
    std::string details;
    std::string state;
    std::string largeImage;
    std::string largeText;
    std::string smallImage;
    std::string smallText;
    int64_t startTimestamp = 0; // unix seconds, 0 = no timer
    int32_t partySize = 0;      // "(size of max)" next to the state; 0 = no party
    int32_t partyMax = 0;
    std::string partyId;        // identifies the party to Discord; empty = a fixed ID
    std::string joinSecret;     // non-empty shows Discord's Join button; handed to the joiner's game

    bool operator==(const Presence& o) const;
    bool operator!=(const Presence& o) const { return !(*this == o); }
};

// Thin wrapper over the Discord Game SDK C API, loaded at runtime so a missing
// discord_game_sdk.dll or a closed Discord client never affects the game.
class DiscordClient 
{
public:
    // searchDir is tried first for discord_game_sdk.dll, then the game folder.
    bool loadSdk(const std::wstring& searchDir);

    // Connects if not connected; returns true while connected. Cheap to call every tick.
    bool ensureConnected(int64_t clientId);
    void runCallbacks();
    void setPresence(const Presence& presence);

    // Called on the joiner's side with the host's joinSecret after they click Join in Discord.
    using JoinHandler = void (*)(const char* secret);
    void setJoinHandler(JoinHandler handler);

private:
    void disconnect();

    void* m_createFn = nullptr; // DiscordCreate, from GetProcAddress
    struct IDiscordCore* m_core = nullptr;
    struct IDiscordActivityManager* m_activities = nullptr;
    unsigned long m_lastAttempt = 0;
    bool m_hasAttempted = false;
    bool m_failureLogged = false;
};
