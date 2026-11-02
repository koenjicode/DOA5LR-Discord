#pragma once

#include <cstdint>
#include <optional>

namespace steam {

// Steam lobby the given user is in, as Steam reports it for "Join Game" (the m_steamIDLobby
// field of ISteamFriends::GetFriendGamePlayed). Asks the game's own steam_api.dll, so it only
// works inside the game process; nullopt if Steam isn't loaded or reports no lobby.
std::optional<uint64_t> lobbyOf(uint64_t steamId);

} // namespace steam
