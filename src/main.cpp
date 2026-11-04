#include "addresses.h"
#include "config.h"
#include "discord.h"
#include "game.h"
#include "log.h"
#include "names.h"
#include "settings.h"
#include "steam.h"

#include <Windows.h>
#include <shellapi.h>
#include <cstdio>
#include <ctime>
#include <iterator>

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
    LOG("p1=%s p1active=%s p1tag=%s p2=%s p2active=%s p2tag=%s step=%s/%s tagSelect=%s stage=%s side=%s mode=%s variant=%s inMatch=%s difficulty=%s replay=%s arcadeStage=%s ui=%s spectating=%s lobby=%s/%s",
        optToString(s.p1Character).c_str(), optToString(s.p1ActiveCharacter).c_str(), optToString(s.p1TagCharacter).c_str(),
        optToString(s.p2Character).c_str(), optToString(s.p2ActiveCharacter).c_str(), optToString(s.p2TagCharacter).c_str(),
        optToString(s.p1PickStep).c_str(), optToString(s.p2PickStep).c_str(), optToString(s.tagSelect).c_str(),
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

// Tag matches: the fighter waiting to tag in (the one of your two picks that isn't on screen).
// nullopt outside tag matches.
std::optional<uint32_t> offScreen(const std::optional<uint32_t>& active, const std::optional<uint32_t>& first,
                                  const std::optional<uint32_t>& tag)
{
    if (!tag || !characterName(*tag)) return std::nullopt;
    return onScreen(active, first) == tag ? first : tag;
}

// Game mode IDs (addr::kGameMode) the presence treats specially.
constexpr uint32_t kVersusMode = 0x01;
constexpr uint32_t kArcadeMode = 0x02;
constexpr uint32_t kTimeAttackMode = 0x03;
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
        const uint32_t ui = s.uiLayer.value_or(0xFF);
        const bool inBattle = s.inMatch.value_or(0) != 0 && s.stage != kNoStage || isBattleScreen(ui);

        // The timer only runs during a battle, restarting with each one.
        if (inBattle && !m_wasInBattle) m_battleStartTime = time(nullptr);
        if (!inBattle && m_wasInBattle) m_battleStartTime = 0;
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
    static bool isRankedLobby(const GameSnapshot& s)
    {
        return s.lobbySlots.value_or(0) > 0 && s.playerLobby.value_or(1) == 0;
    }
    
    static bool isP2(const GameSnapshot& s)
    {
        return s.localPlayerSide.value_or(0) == 1;
    }
    
    // Checks if a character is registered in both tag slots.
    static bool isTag(const GameSnapshot& s)
    {
        return s.p1TagCharacter != kNoCharacter && s.p2TagCharacter != kNoCharacter || s.tagSelect.value_or(0) == 1;
    }
    
    static int getLocalControllerCount(const GameSnapshot& s)
    {
        int count = 0;
        if (s.p1Controller.value_or(0xFF) != 0xFF) {count += 1;}
        if (s.p1PartnerController.value_or(0xFF) != 0xFF) {count += 1;}
        if (s.p2Controller.value_or(0xFF) != 0xFF) {count += 1;}
        if (s.p2PartnerController.value_or(0xFF) != 0xFF) {count += 1;}
        return count;
    }
    
    static bool isOnlyLocal(const GameSnapshot& s)
    {
        return getLocalControllerCount(s) <= 1;
    }
    
    static bool isLocalVersus(const GameSnapshot& s)
    {
        bool p1Human = s.p1Controller.value_or(0xFF) != 0xFF || s.p1PartnerController.value_or(0xFF) != 0xFF;
        bool p2Human = s.p2Controller.value_or(0xFF) != 0xFF || s.p2PartnerController.value_or(0xFF) != 0xFF;
        return p1Human && p2Human;
    }

    // is a player lobby.
    static bool isInLobby(const GameSnapshot& s)
    {
        return s.uiLayer.value_or(0) != 21 && !isRankedLobby(s) && s.lobbySlots.value_or(0) > 0;
    }

    // State line on every screen: where you're playing. Online lobbies read "In a Lobby" so Discord's
    // party size reads naturally after it; offline fights with progress show that instead.
    std::string locationLabel(const GameSnapshot& s, bool inBattle) const
    {
        if (isInLobby(s)) return "In a Lobby";
        if (isRankedLobby(s)) return "Online";
        if (!inBattle) return s.uiLayer && isOnlineScreen(*s.uiLayer) ? "Online" : "Offline";

        if (s.replay.value_or(0) != 0) return "Offline";
        if (s.gameMode == kOnlineMode) return "Online";
        
        // If we're in any game mode that tracks a value, mark down the progress (if it's valid)
        if (s.gameMode == kArcadeMode || s.gameMode == kTimeAttackMode || s.gameMode == kSurvivalMode)
        {
            const std::string progress = progressLabel(s);
            if (!progress.empty()) return progress;
        }

        if (s.gameMode == kVersusMode)
        {
            if (!isLocalVersus(s)) { return isDuo(s) ? "VS CPU w/ Partner" : "VS CPU"; }
            return"Local Match";
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
        std::optional<uint32_t> main;   // your first character (the cursor, while picking it)
        std::optional<uint32_t> partner; // the tag partner under the cursor, while picking it
        bool dualSelection = false; // are both players selecting at the same time.
        bool choosingPartner = false; // choosing partner character.
    };
    
    static bool isDuo(const GameSnapshot& s)
    {
        if (isP2(s)) { return s.p2Controller.value_or(0xFF) != 0xFF && s.p2PartnerController.value_or(0xFF) != 0xFF; }
        return s.p1Controller.value_or(0xFF) != 0xFF && s.p1PartnerController.value_or(0xFF) != 0xFF;
    }

    // Reads your side's pick: the slot follows the cursor while you pick your first character,
    // and the tag slot follows it while you pick your partner.
    CharacterPick characterPick(const GameSnapshot& s) const
    {
        const bool onP2Side = isP2(s);
        const auto& step = onP2Side ? s.p2PickStep : s.p1PickStep;

        CharacterPick pick;
        pick.main = onP2Side ? s.p2Character : s.p1Character;
        pick.partner = onP2Side ? s.p2TagCharacter : s.p1TagCharacter;
        pick.dualSelection = isDuo(s);
        pick.choosingPartner = s.tagSelect.value_or(0) == 1 && step.value_or(0xFF) == 1;
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
        
        Presence p;
        if (isLocalVersus(s))
        {
            p.details = "Choosing Characters";
        }
        else if (isTag(s))
        {
            if (pick.dualSelection)
            {
               p.details = "Choosing a Tag Team";
            }
            else
            {
                p.details = pick.choosingPartner ? "Choosing a Partner" : "Choosing a Main Character";
            }
        }
        else
        {
            p.details = "Choosing a Character";
        }
        
        m_lastMain = pick.main != kNoCharacter ? pick.main : m_lastMain;
        m_lastPartner = pick.partner != kNoCharacter ? pick.partner : m_lastPartner;
        
        // Build a main character and the partner underneath them.
        if (isOnlyLocal(s) && isKnownCharacter(m_lastMain))
        {
            buildLargeCharacterImage(m_lastMain, p);
            if (pick.choosingPartner && isKnownCharacter(m_lastPartner))
            {
                buildCharacterImage(m_lastPartner, p);
            }
        }
        else
        {
            buildDefaultImage(p);
        }

        return p;
    }

    // Stage select. The stage address isn't set until the fight loads either.
    Presence buildStageSelect(const GameSnapshot& s)
    {
        const char* stage = s.stage ? stageName(*s.stage) : nullptr;
        
        Presence p;
        p.details = isRankedLobby(s) ? "Randomizing Stage" : "Choosing a Stage";
        buildStageImage(s, stage, p);
        
        return p;
    }

    void buildDefaultImage(Presence& p)
    {
        p.largeImage = coverImage(config::get().cover);
        p.largeText = std::string("DOA5LRDiscord v.") + settings::kVersion;
    }

    static const char* coverImage(config::Cover cover)
    {
        switch (cover)
        {
        case config::Cover::DefaultJp: return settings::kDefaultJpLargeImage;
        case config::Cover::Ultimate: return settings::kUltimateLargeImage;
        case config::Cover::UltimateJp: return settings::kUltimateJpLargeImage;
        case config::Cover::Vanilla: return settings::kVanillaLargeImage;
        default: return settings::kDefaultLargeImage;
        }
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

    // Builds character images but with a "large" postfix.
    void buildLargeCharacterImage(std::optional<uint32_t> character, Presence& p)
    {
        p.largeImage = assetKey(characterName(*character)) + "_large";
        p.largeText = characterLabel(*character);
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
        p.startTimestamp = m_battleStartTime;

        const bool isReplay = s.replay.value_or(0) != 0;
        // Replays, Watch Mode and online matches you're only watching have no character of yours.
        const bool spectating = isReplay || (s.gameMode && isWatchMode(*s.gameMode)) ||
                                s.spectating.value_or(0) != 0;

        if (config::get().largeCharacterIcons)
        {
            if (!spectating && isOnlyLocal(s) && hasChar && characterName(*mainChar))
            {
                buildLargeCharacterImage(mainChar, p);
                if (stage) p.largeText += std::string(" in ") + stage;
                if (isTag(s))
                {
                    auto partnerChar = onP2Side ? offScreen(s.p2ActiveCharacter, s.p2Character, s.p2TagCharacter) : offScreen(s.p1ActiveCharacter, s.p1Character, s.p1TagCharacter);
                    if (isKnownCharacter(partnerChar))
                    {
                        buildCharacterImage(partnerChar, p);
                    }
                }
            }
            else
            {
                buildStageImage(s, stage, p);
            }
        }
        else
        {
            buildStageImage(s, stage, p);
            if (!spectating && isOnlyLocal(s) && hasChar && characterName(*mainChar)) {
                buildCharacterImage(mainChar, p);
            }
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
            if (isRankedLobby(s)) return "Playing Rank Match";
            if (!isInLobby(s)) return "Playing Online Match";
            return s.spectating.value_or(0) != 0 ? "Spectating Lobby Match" : "Playing Lobby Match";
        }
        
        // TODO: At a later date, combine isTag into the gameModeName check to better support localised names.
        const bool tag = isTag(s);
        const char* name = gameModeName(mode, s.modeVariant);
        const std::string label = name ? name : hexId("Game Mode", mode);
        if (isWatchMode(mode)) return "In " + label;
        if (mode == kTutorialMode) return "Playing the " + label;
        if (mode == kTrainingMode || mode == kComboChallengeMode)
        {
            const std::string tagLabel = !tag ? "Solo" : "Tag";
            if (mode == kTrainingMode && s.modeVariant != 1) { return "Practicing " + tagLabel + " in " + label; }
            return "Practicing in " + label;
        }
        if (tag) { return "Playing Tag " + label; }
        return "Playing " + label;
    }
    
    std::string buildArcadeStageLabel(const uint32_t count) const
    {
        return "Stage " + std::to_string(count + 1);
    }
    
    std::string buildWinLabel(const uint32_t count) const
    {
        const uint32_t wins = count > 0 ? count - 1 : 0;
        const std::string textAmount = std::to_string(wins);
        if (wins == 1) { return textAmount + " Win"; }
        return textAmount + " Wins";
    }

    // Return the current level of progress. 
    // For Arcade and Time Attack it will present the Stage with the difficulty. "Stage 4 (Master)".
    // For Survival, it will display your current number of wins.
    std::string progressLabel(const GameSnapshot& s) const
    {
        std::string difficulty;
        if (s.difficulty) {
            const char* name = difficultyName(*s.difficulty);
            difficulty = name ? name : "Level " + std::to_string(*s.difficulty + 1);
        }
        
        const uint32_t count = *s.arcadeStage;
        const bool isSurvival = s.gameMode == kSurvivalMode;
        
        std::string label = !isSurvival ? buildArcadeStageLabel(count) : buildWinLabel(count);
        if (!difficulty.empty()) label += " (" + difficulty + ")";
        return label;
    }

    bool m_wasInBattle = false;
    int64_t m_battleStartTime = 0;
    std::optional<uint32_t> m_lastPartner; // last real character under your cursor on character select
    std::optional<uint32_t> m_lastMain;    // last real first pick on character select
};

// In a lobby Steam knows about, lets Discord show a Join button. The secret is "<lobby>/<host>",
// the two IDs of the game's own steam://joinlobby invite link. Steam takes a while after the
// lobby is made to report it, so there's no button until then.
void buildJoin(Presence& p)
{
    const uint64_t self = readGame64(addr::kLocalSteamId).value_or(0);
    const auto lobby = self ? steam::lobbyOf(self) : std::nullopt;
    if (!lobby) return;

    p.partyId = std::to_string(*lobby);
    p.joinSecret = p.partyId + "/" + std::to_string(self);
}

// Joiner's side of Discord's Join button: opens the host's lobby through Steam. The secret comes
// from another player, so only two plain numbers are accepted, never an arbitrary link.
void onJoin(const char* secret)
{
    const std::string s = secret ? secret : "";
    const size_t slash = s.find('/');
    const auto isNumber = [](const std::string& part) {
        return !part.empty() && part.size() <= 20 &&
               part.find_first_not_of("0123456789") == std::string::npos;
    };
    if (slash == std::string::npos || !isNumber(s.substr(0, slash)) || !isNumber(s.substr(slash + 1)))
    {
        LOG("Ignoring malformed join secret");
        return;
    }

    const std::string link = std::string("steam://joinlobby/") + settings::kSteamAppId + "/" + s;
    LOG("Joining %s", link.c_str());
    ShellExecuteA(nullptr, "open", link.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

DWORD WINAPI worker(LPVOID) 
{
    const std::wstring dir = moduleDir(g_module);
    if (settings::kWriteLog) logger::open(dir);
    LOG("DOA5Discord v%s loaded, client id %lld", settings::kVersion, settings::kClientId);
    config::load(dir);

    // Give the game a moment to finish starting before touching anything.
    Sleep(3000);

    DiscordClient discord;
    const bool sdkLoaded = discord.loadSdk(dir);
    discord.setJoinHandler(onJoin);

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

        if (presence.partySize > 0) buildJoin(presence);

        if (sdkLoaded && discord.ensureConnected(settings::kClientId))
        {
            // Resend after a reconnect even if nothing changed.
            if (!wasConnected || firstTick || presence != lastPresence) 
            {
                LOG("presence: details='%s' state='%s' large='%s' small='%s' start=%lld join='%s'",
                    presence.details.c_str(), presence.state.c_str(), presence.largeImage.c_str(),
                    presence.smallImage.c_str(), presence.startTimestamp, presence.joinSecret.c_str());
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
