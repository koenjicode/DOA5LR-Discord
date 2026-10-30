#include "discord.h"
#include "game.h"
#include "log.h"
#include "names.h"
#include "settings.h"

#include <Windows.h>
#include <cstdio>
#include <ctime>

namespace {

HMODULE g_module = nullptr;

std::wstring moduleDir(HMODULE module) 
{
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(module, path, MAX_PATH);
    std::wstring dir(path, len);
    size_t slash = dir.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"." : dir.substr(0, slash);
}

std::string hexId(const char* prefix, uint32_t id) 
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%s (0x%02X)", prefix, id);
    return buf;
}

std::string characterLabel(uint32_t id) 
{
    const char* name = characterName(id);
    return name ? name : hexId("Unknown Character", id);
}

std::string optToString(const std::optional<uint32_t>& v) 
{
    if (!v) return "-";
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%X", *v);
    return buf;
}

void logSnapshot(const GameSnapshot& s) 
{
    LOG("p1=%s p1active=%s p2=%s p2active=%s stage=%s side=%s mode=%s variant=%s inMatch=%s difficulty=%s replay=%s",
        optToString(s.p1Character).c_str(), optToString(s.p1ActiveCharacter).c_str(),
        optToString(s.p2Character).c_str(), optToString(s.p2ActiveCharacter).c_str(),
        optToString(s.stage).c_str(), optToString(s.localPlayerSide).c_str(),
        optToString(s.gameMode).c_str(), optToString(s.modeVariant).c_str(),
        optToString(s.inMatch).c_str(), optToString(s.difficulty).c_str(), optToString(s.replay).c_str());
}

// Prefers whoever is on screen, so tag battles follow the active partner.
std::optional<uint32_t> onScreen(const std::optional<uint32_t>& active, const std::optional<uint32_t>& slot) 
{
    return active && characterName(*active) ? active : slot;
}

class PresenceBuilder 
{
public:
    Presence build(const GameSnapshot& s) 
    {
        const bool onP2Side = s.localPlayerSide.value_or(0) == 1;
        auto mainChar = onP2Side ? onScreen(s.p2ActiveCharacter, s.p2Character) : onScreen(s.p1ActiveCharacter, s.p1Character);
        const bool hasChar = mainChar && *mainChar != kNoCharacter;
        const char* stage = s.stage ? stageName(*s.stage) : nullptr;
        const bool inMatch = s.inMatch.value_or(0) != 0;

        if (inMatch && !m_wasInMatch) m_matchStart = static_cast<int64_t>(time(nullptr));
        if (!inMatch) m_matchStart = 0;
        m_wasInMatch = inMatch;

        Presence p;
        p.details = modeLabel(s, inMatch);
        p.startTimestamp = m_matchStart;

        const bool isReplay = s.replay.value_or(0) != 0;
        const bool spectating = isReplay || (s.gameMode && isSpectatorMode(*s.gameMode));

        std::string charText;
        if (spectating) {
            auto p1 = onScreen(s.p1ActiveCharacter, s.p1Character);
            auto p2 = onScreen(s.p2ActiveCharacter, s.p2Character);
            if (p1 && *p1 != kNoCharacter && p2 && *p2 != kNoCharacter) {
                charText = characterLabel(*p1) + " vs " + characterLabel(*p2);
            }
        } else if (hasChar) {
            charText = characterLabel(*mainChar);
        }
        
        // TODO : Change what shows in the state, maybe.
        /**
        if (inMatch) {
            p.state = charText;
            if (stage) p.state += p.state.empty() ? stage : std::string(" - ") + stage;
        }
        */

        // Set Stage Image and Character Text.
        const char* image = inMatch && stage ? stageImage(*s.stage) : nullptr;
        p.largeImage = image ? image : settings::kDefaultLargeImage;
        p.largeText = inMatch && stage ? stage : "DEAD OR ALIVE 5 Last Round";
        if (inMatch && !spectating && hasChar && characterName(*mainChar)) {
            p.smallImage = assetKey(characterName(*mainChar));
            p.smallText = charText;
        }
        
        return p;
    }

private:
    std::string modeLabel(const GameSnapshot& s, bool inMatch) const 
    {
        // The mode byte keeps the previous mode until the next fight starts.
        if (!inMatch) return "Waiting";
        if (s.replay.value_or(0) != 0) return "Watching a Replay";
        if (!s.gameMode) return "In a Match";

        const char* mode = gameModeName(*s.gameMode, s.modeVariant);
        std::string label = mode ? mode : hexId("Game Mode", *s.gameMode);
        // Only read for modes that have a difficulty (addr::difficultyFor).
        if (s.difficulty) {
            const char* difficulty = difficultyName(*s.difficulty);
            label += " (" + (difficulty ? std::string(difficulty) : "Level " + std::to_string(*s.difficulty + 1)) + ")";
        }
        return label;
    }

    bool m_wasInMatch = false;
    int64_t m_matchStart = 0;
};

DWORD WINAPI worker(LPVOID) 
{
    const std::wstring dir = moduleDir(g_module);
    if (settings::kWriteLog) logger::open(dir);
    LOG("DOA5Discord loaded, client id %lld", settings::kClientId);

    // Give the game a moment to finish starting before touching anything.
    Sleep(3000);

    DiscordClient discord;
    const bool sdkLoaded = discord.loadSdk(dir);

    PresenceBuilder builder;
    GameSnapshot lastSnapshot;
    Presence lastPresence;
    bool firstTick = true;
    bool wasConnected = false;

    for (;;) 
    {
        GameSnapshot snapshot = readGame();
        if (firstTick || snapshot != lastSnapshot) logSnapshot(snapshot);
        lastSnapshot = snapshot;

        Presence presence = builder.build(snapshot);

        if (sdkLoaded && discord.ensureConnected(settings::kClientId))
        {
            // Resend after a reconnect even if nothing changed.
            if (!wasConnected || firstTick || presence != lastPresence) 
            {
                LOG("presence: details='%s' state='%s' large='%s' small='%s' start=%lld",
                    presence.details.c_str(), presence.state.c_str(), presence.largeImage.c_str(),
                    presence.smallImage.c_str(), presence.startTimestamp);
                discord.setPresence(presence);
                lastPresence = presence;
                firstTick = false;
            }
            wasConnected = true;
            discord.runCallbacks();
        } else 
        {
            wasConnected = false;
        }

        Sleep(settings::kUpdateIntervalMs);
    }
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
