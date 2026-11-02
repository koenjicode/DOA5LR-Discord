#include "addresses.h"
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
    LOG("p1=%s p1active=%s p2=%s p2active=%s stage=%s side=%s mode=%s variant=%s inMatch=%s difficulty=%s replay=%s arcadeStage=%s ui=%s spectating=%s lobby=%s/%s",
        optToString(s.p1Character).c_str(), optToString(s.p1ActiveCharacter).c_str(),
        optToString(s.p2Character).c_str(), optToString(s.p2ActiveCharacter).c_str(),
        optToString(s.stage).c_str(), optToString(s.localPlayerSide).c_str(),
        optToString(s.gameMode).c_str(), optToString(s.modeVariant).c_str(),
        optToString(s.inMatch).c_str(), optToString(s.difficulty).c_str(), optToString(s.replay).c_str(), optToString(s.arcadeStage).c_str(),
        optToString(s.uiLayer).c_str(), optToString(s.spectating).c_str(), optToString(s.lobbyMembers).c_str(), optToString(s.lobbySlots).c_str());
}

// Prefers whoever is on screen, so tag battles follow the active partner.
std::optional<uint32_t> onScreen(const std::optional<uint32_t>& active, const std::optional<uint32_t>& slot) 
{
    return active && characterName(*active) ? active : slot;
}

// Game mode IDs (addr::kGameMode) the presence treats specially.
constexpr uint32_t kVersusMode = 0x01;
constexpr uint32_t kArcadeMode = 0x02;
constexpr uint32_t kSurvivalMode = 0x04;
constexpr uint32_t kTrainingMode = 0x05;
constexpr uint32_t kOnlineMode = 0x08;
constexpr uint32_t kTutorialMode = 0x11;
constexpr uint32_t kComboChallengeMode = 0x12;

// addr::kP1Controller / kP2Controller value for a CPU-controlled side.
constexpr uint32_t kCpuController = 0xFF;

// Builds the Discord presence for whichever screen the game is on. build() picks the screen;
// each build*() function fills in the presence for one of them.
class PresenceBuilder
{
public:
    Presence build(const GameSnapshot& s)
    {
        // The fight flag is set as soon as loading starts, while the stage and characters are still
        // stale; the UI layer stays at kLoading until the fight is actually loaded.
        const uint32_t ui = s.uiLayer.value_or(0);
        const bool inBattle = s.inMatch.value_or(0) != 0 && s.stage != kNoStage;

        // The timer only runs during a battle, restarting with each one.
        if (inBattle && !m_wasInBattle) m_battleStart = static_cast<int64_t>(time(nullptr));
        if (!inBattle) m_battleStart = 0;
        m_wasInBattle = inBattle;

        Presence p;
        if (inBattle) p = buildBattle(s);
        else if (ui == addr::ui::kCharacterSelect) p = buildCharacterSelect(s);
        else if (ui == addr::ui::kStageSelect) p = buildStageSelect(s);
        else if (isSearching(s)) p = buildSearch(s);
        else p = buildMenu(s);

        p.state = locationLabel(s, inBattle);
        if (isInLobby(s)) buildLobby(s, p);
        return p;
    }

private:
    // is searching for a match in ranked queue.
    static bool isSearching(const GameSnapshot& s)
    {
        return s.uiLayer.value_or(0) == 21 && s.lobbySlots.value_or(0) > 0;
    }
    
    // Is ranked match: ranked runs on a hidden 2-slot lobby that isn't a player-made one.
    static bool isRanked(const GameSnapshot& s)
    {
        return s.lobbySlots.value_or(0) > 0 && s.playerLobby.value_or(1) == 0;
    }

    // is a player lobby.
    static bool isInLobby(const GameSnapshot& s)
    {
        return s.uiLayer.value_or(0) != 21 && !isRanked(s) && s.lobbySlots.value_or(0) > 0;
    }

    // State line on every screen: where you're playing. Online lobbies read "In a Lobby" so Discord's
    // party size reads naturally after it; offline fights with progress show that instead.
    std::string locationLabel(const GameSnapshot& s, bool inBattle) const
    {
        if (isInLobby(s)) return "In a Lobby";
        if (isRanked(s)) return "Online";
        if (!inBattle) return s.uiLayer && isOnlineScreen(*s.uiLayer) ? "Online" : "Offline";

        if (s.replay.value_or(0) != 0) return "Offline";
        if (s.gameMode == kOnlineMode) return "Online";

        const std::string progress = progressLabel(s);
        if (!progress.empty()) return progress;

        if (s.gameMode == kVersusMode)
        {
            const bool cpu = s.p1Controller.value_or(0) == kCpuController ||
                             s.p2Controller.value_or(0) == kCpuController;
            return cpu ? "Vs CPU" : "Local Match";
        }
        return "Offline";
    }

    // Online lobby: people in the lobby out of its slots, shown as the party size.
    void buildLobby(const GameSnapshot& s, Presence& p)
    {
        if (!s.lobbyMembers || *s.lobbyMembers == 0) return;
        p.partySize = static_cast<int32_t>(*s.lobbyMembers);
        p.partyMax = static_cast<int32_t>(*s.lobbySlots);
    }

    // What your side is doing on character select.
    struct CharacterPick
    {
        std::optional<uint32_t> first;   // your first character (the cursor, while picking it)
        std::optional<uint32_t> partner; // the tag partner under the cursor, while picking it
        bool choosingPartner = false;
    };

    // Reads your side's pick: the slot follows the cursor while you pick your first character,
    // and the tag slot follows it while you pick your partner.
    CharacterPick characterPick(const GameSnapshot& s) const
    {
        const bool onP2Side = s.localPlayerSide.value_or(0) == 1;
        const auto& step = onP2Side ? s.p2PickStep : s.p1PickStep;

        CharacterPick pick;
        pick.first = onP2Side ? s.p2Character : s.p1Character;
        pick.partner = onP2Side ? s.p2TagCharacter : s.p1TagCharacter;
        pick.choosingPartner = step.value_or(0xFF) == 1 && pick.partner != kNoCharacter;
        return pick;
    }

    static bool isKnownCharacter(const std::optional<uint32_t>& id)
    {
        return id && characterName(*id);
    }

    // Character select: whoever is under your cursor, and your partner once you're picking one.
    Presence buildCharacterSelect(const GameSnapshot& s)
    {
        const CharacterPick pick = characterPick(s);

        // The cursor is the partner while picking one, otherwise the first character. Keep the
        // last real pick so the image doesn't flicker when a slot briefly reads empty.
        auto selectChar = pick.choosingPartner ? pick.partner : pick.first;
        if (isKnownCharacter(selectChar)) m_lastSelected = selectChar;
        else selectChar = m_lastSelected;

        Presence p;
        p.details = pick.choosingPartner ? "Choosing a Partner" : "Choosing a Character";
        buildDefaultImage(p);

        if (isKnownCharacter(selectChar))
        {
            p.smallImage = assetKey(characterName(*selectChar));
            p.smallText = characterLabel(*selectChar);
        }

        return p;
    }

    // Stage select. The stage address isn't set until the fight loads either.
    Presence buildStageSelect(const GameSnapshot& s)
    {
        const char* stage = s.stage ? stageName(*s.stage) : nullptr;
        
        Presence p;
        p.details = isRanked(s) ? "Randomizing Stage" : "Choosing a Stage";
        buildStageImage(s, stage, p);
        
        if (isKnownCharacter(m_lastSelected))
        {
            buildCharacterImage(m_lastSelected, p);
        }
        
        return p;
    }

    void buildDefaultImage(Presence& p)
    {
        p.largeImage = settings::kDefaultLargeImage;
        p.largeText = "DOA5LRDiscord v.1.3.0";
    }
    
    // Build search data.
    Presence buildSearch(const GameSnapshot& s)
    {
        Presence p;
        bool hasOpponent = s.lobbyMembers.value_or(0) == 2;
        p.details = hasOpponent ? "Connecting with Opponent" : "Searching for Rank Match...";
        buildDefaultImage(p);
        return p;
    }

    // Main menu, results and any screen without its own function: the screen's name, if known.
    Presence buildMenu(const GameSnapshot& s)
    {
        Presence p;
        const char* screen = s.uiLayer ? screenName(*s.uiLayer) : nullptr;
        p.details = screen ? screen : "Waiting";
        buildDefaultImage(p);
        return p;
    }

    void buildStageImage(const GameSnapshot& s, const char* stage, Presence& p)
    {
        if (const char* image = stage ? stageImage(*s.stage) : nullptr)
        {
            p.largeImage = image;
            p.largeText = stage;
        }
        else
        {
            buildDefaultImage(p);
        }
    }

    void buildCharacterImage(std::optional<uint32_t> mainChar, Presence& p)
    {
        p.smallImage = assetKey(characterName(*mainChar));
        p.smallText = characterLabel(*mainChar);
    }

    // A loaded fight (or replay / Watch Mode): mode, stage image, your character and the timer.
    Presence buildBattle(const GameSnapshot& s)
    {
        const bool onP2Side = s.localPlayerSide.value_or(0) == 1;
        auto mainChar = onP2Side ? onScreen(s.p2ActiveCharacter, s.p2Character) : onScreen(s.p1ActiveCharacter, s.p1Character);
        const bool hasChar = mainChar && *mainChar != kNoCharacter;
        const char* stage = s.stage ? stageName(*s.stage) : nullptr;

        Presence p;
        p.details = battleLabel(s);
        p.startTimestamp = m_battleStart;

        const bool isReplay = s.replay.value_or(0) != 0;
        // Replays, Watch Mode and online matches you're only watching have no character of yours.
        const bool spectating = isReplay || (s.gameMode && isWatchMode(*s.gameMode)) ||
                                s.spectating.value_or(0) != 0;

        buildStageImage(s, stage, p);
        if (!spectating && hasChar && characterName(*mainChar)) {
            buildCharacterImage(mainChar, p);
        }

        return p;
    }

    // Details line during a battle: what you're doing in which mode, e.g. "Playing Arcade Mode".
    std::string battleLabel(const GameSnapshot& s) const
    {
        if (s.replay.value_or(0) != 0) return "Watching a Replay";
        if (!s.gameMode) return "In a Match";

        const uint32_t mode = *s.gameMode;
        if (mode == kOnlineMode)
        {
            if (isRanked(s)) return "Playing Rank Match";
            if (!isInLobby(s)) return "Playing Online Match";
            return s.spectating.value_or(0) != 0 ? "Spectating Lobby Match" : "Playing Lobby Match";
        }

        const char* name = gameModeName(mode, s.modeVariant);
        const std::string label = name ? name : hexId("Game Mode", mode);
        if (isWatchMode(mode)) return "In " + label;
        if (mode == kTutorialMode) return "Playing the " + label;
        if (mode == kTrainingMode || mode == kComboChallengeMode) return "Practicing in " + label;
        return "Playing " + label;
    }

    // State line during a battle: "Stage 4 (Master)" in Arcade, "Defeated 14 (Master)" in Survival,
    // just the difficulty in Time Attack, and empty otherwise.
    std::string progressLabel(const GameSnapshot& s) const
    {
        // Only read for modes that have a difficulty (addr::difficultyFor).
        std::string difficulty;
        if (s.difficulty) {
            const char* name = difficultyName(*s.difficulty);
            difficulty = name ? name : "Level " + std::to_string(*s.difficulty + 1);
        }

        // The same counter numbers Arcade stages from 0, and in Survival runs one ahead of the
        // opponents beaten (it goes up as soon as the next opponent jumps in).
        const bool arcade = s.gameMode == kArcadeMode;
        const bool survival = s.gameMode == kSurvivalMode;
        if (!(arcade || survival) || !s.arcadeStage) return difficulty;

        const uint32_t count = *s.arcadeStage;
        std::string label = arcade ? "Stage " + std::to_string(count + 1)
                                   : "Defeated " + std::to_string(count > 0 ? count - 1 : 0);
        if (!difficulty.empty()) label += " (" + difficulty + ")";
        return label;
    }

    bool m_wasInBattle = false;
    int64_t m_battleStart = 0;
    std::optional<uint32_t> m_lastSelected; // last real character under your cursor on character select
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
