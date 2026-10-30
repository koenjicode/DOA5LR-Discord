#include "discord.h"
#include "log.h"

#include <discord_game_sdk.h>

namespace {

using DiscordCreateFn = enum EDiscordResult(DISCORD_API*)(DiscordVersion, struct DiscordCreateParams*,
                                                         struct IDiscordCore**);

constexpr DWORD kReconnectDelayMs = 10000;

void copyField(char (&dst)[128], const std::string& src) 
{
    strncpy_s(dst, src.c_str(), _TRUNCATE);
}

void DISCORD_CALLBACK onActivityUpdated(void*, enum EDiscordResult result) 
    {
    if (result != DiscordResult_Ok) LOG("update_activity failed: %d", static_cast<int>(result));
}

} // namespace

bool Presence::operator==(const Presence& o) const 
{
    return details == o.details && state == o.state && largeImage == o.largeImage &&
           largeText == o.largeText && smallImage == o.smallImage && smallText == o.smallText &&
           startTimestamp == o.startTimestamp;
}

bool DiscordClient::loadSdk(const std::wstring& searchDir)
{
    HMODULE sdk = LoadLibraryW((searchDir + L"\\discord_game_sdk.dll").c_str());
    if (!sdk) sdk = LoadLibraryW(L"discord_game_sdk.dll");
    if (!sdk)
    {
        LOG("discord_game_sdk.dll not found (error %lu)", GetLastError());
        return false;
    }
    m_createFn = reinterpret_cast<void*>(GetProcAddress(sdk, "DiscordCreate"));
    if (!m_createFn)
    {
        LOG("DiscordCreate export missing from discord_game_sdk.dll");
        return false;
    }
    return true;
}

bool DiscordClient::ensureConnected(int64_t clientId)
{
    if (m_core) return true;
    if (!m_createFn) return false;

    DWORD now = GetTickCount();
    if (m_hasAttempted && now - m_lastAttempt < kReconnectDelayMs) return false;
    m_hasAttempted = true;
    m_lastAttempt = now;

    // SetDefault zeroes the struct, so it has to come before the fields are filled in.
    DiscordCreateParams params;
    DiscordCreateParamsSetDefault(&params);
    params.client_id = clientId;
    params.flags = DiscordCreateFlags_NoRequireDiscord; // never let the SDK close the game

    auto create = reinterpret_cast<DiscordCreateFn>(m_createFn);
    EDiscordResult result = create(DISCORD_VERSION, &params, &m_core);
    if (result != DiscordResult_Ok || !m_core) 
    {
        if (!m_failureLogged) LOG("DiscordCreate failed: %d (is Discord running?)", static_cast<int>(result));
        m_failureLogged = true;
        m_core = nullptr;
        return false;
    }

    m_activities = m_core->get_activity_manager(m_core);
    m_failureLogged = false;
    LOG("Connected to Discord");
    return true;
}

void DiscordClient::runCallbacks() 
{
    if (!m_core) return;
    EDiscordResult result = m_core->run_callbacks(m_core);
    if (result != DiscordResult_Ok) 
    {
        LOG("Lost connection to Discord: %d", static_cast<int>(result));
        disconnect();
    }
}

void DiscordClient::setPresence(const Presence& p) 
{
    if (!m_activities) return;

    DiscordActivity activity{};
    activity.type = DiscordActivityType_Playing;
    copyField(activity.details, p.details);
    copyField(activity.state, p.state);
    copyField(activity.assets.large_image, p.largeImage);
    copyField(activity.assets.large_text, p.largeText);
    copyField(activity.assets.small_image, p.smallImage);
    copyField(activity.assets.small_text, p.smallText);
    activity.timestamps.start = p.startTimestamp;

    m_activities->update_activity(m_activities, &activity, nullptr, onActivityUpdated);
}

void DiscordClient::disconnect() 
{
    if (m_core) m_core->destroy(m_core);
    m_core = nullptr;
    m_activities = nullptr;
}
