#include "steam.h"

#include <Windows.h>

namespace {

// FriendGameInfo_t from the Steamworks SDK.
struct FriendGameInfo
{
    uint64_t gameId;
    uint32_t gameIp;
    uint16_t gamePort;
    uint16_t queryPort;
    uint64_t lobbyId;
};

// The start of ISteamFriends' vtable. These first entries have kept their order in every
// interface version, so this works with the game's old steam_api.dll as well as newer ones.
class ISteamFriends
{
public:
    virtual const char* GetPersonaName() = 0;
    virtual void SetPersonaName(const char* name) = 0;
    virtual int GetPersonaState() = 0;
    virtual int GetFriendCount(int flags) = 0;
    virtual uint64_t GetFriendByIndex(int index, int flags) = 0;
    virtual int GetFriendRelationship(uint64_t steamId) = 0;
    virtual int GetFriendPersonaState(uint64_t steamId) = 0;
    virtual const char* GetFriendPersonaName(uint64_t steamId) = 0;
    virtual bool GetFriendGamePlayed(uint64_t steamId, FriendGameInfo* info) = 0;
};

using SteamFriendsFn = ISteamFriends* (*)();

// Kept free of C++ objects so __try can be used.
bool queryGamePlayed(uint64_t steamId, FriendGameInfo* info)
{
    HMODULE api = GetModuleHandleW(L"steam_api.dll");
    if (!api) return false;
    auto steamFriends = reinterpret_cast<SteamFriendsFn>(GetProcAddress(api, "SteamFriends"));
    if (!steamFriends) return false;

    __try
    {
        ISteamFriends* friends = steamFriends();
        return friends && friends->GetFriendGamePlayed(steamId, info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

} // namespace

namespace steam {

std::optional<uint64_t> lobbyOf(uint64_t steamId)
{
    FriendGameInfo info{};
    if (!queryGamePlayed(steamId, &info) || info.lobbyId == 0) return std::nullopt;
    return info.lobbyId;
}

} // namespace steam
