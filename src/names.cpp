#include "names.h"

#include <cctype>
#include <unordered_map>

namespace {

// Character IDs
const std::unordered_map<uint32_t, const char*> kCharacters = {
    {0x00, "Zack"},       {0x01, "Tina"},        {0x02, "Jann Lee"},   {0x03, "Ein"},
    {0x04, "Hayabusa"},   {0x05, "Kasumi"},      {0x06, "Gen Fu"},     {0x07, "Helena"},
    {0x08, "Leon"},       {0x09, "Bass"},        {0x0A, "Kokoro"},     {0x0B, "Hayate"},
    {0x0C, "Leifang"},    {0x0D, "Ayane"},       {0x0E, "Eliot"},      {0x0F, "Lisa/La Mariposa"},
    {0x10, "Alpha-152"},  {0x13, "Brad Wong"},   {0x14, "Christie"},   {0x15, "Hitomi"},
    {0x18, "Bayman"},     {0x1D, "Rig"},         {0x1E, "Mila"},       {0x1F, "Akira"},
    {0x20, "Sarah"},      {0x21, "Pai"},         {0x27, "Momiji"},     {0x28, "Rachel"},
    {0x29, "Jacky"},      {0x2A, "Marie Rose"},  {0x2B, "Phase-4"},    {0x2C, "Nyotengu"},
    {0x2D, "Honoka"},     {0x2E, "Raidou"},      {0x2F, "Naotora"},    {0x30, "Mai"},
};

// Stage IDs
const std::unordered_map<uint32_t, const char*> kStages = {
    {0x01, "Scramble 1"},             {0x02, "Scramble 2"},
    {0x03, "Fuel 1"},                 {0x04, "Fuel 2"},
    {0x05, "Lab (Closed Capsules)"},  {0x06, "Lab (Open Capsules)"},
    {0x07, "Lab (Destroyed)"},        {0x08, "Flow 1"},
    {0x09, "Flow 2"},                 {0x0A, "Sanctuary"},
    {0x0B, "Home 1"},                 {0x0C, "Home 2"},
    {0x0D, "Street 1"},               {0x0E, "Street 2"},
    {0x0F, "The Show"},               {0x10, "The Tiger Show"},
    {0x11, "Coming Soon"},            {0x13, "Hotzone"},
    {0x14, "The Ends of the Earth"},  {0x16, "Depth"},
    {0x18, "Temple of the Dragon"},   {0x19, "Sakura"},
    {0x1A, "Fighting Entertainment"}, {0x1B, "Dojo"},
    {0x1D, "Gym"},                    {0x1E, "Dead or Alive"},
    {0x1F, "Arrival"},                {0x21, "Zack Island"},
    {0x22, "Forest"},                 {0x23, "Sky City Tokyo"},
    {0x24, "Desert"},                 {0x25, "Lost World"},
    {0x26, "Aircraft Carrier"},       {0x27, "Lorelei 1"},
    {0x28, "Lorelei 2"},              {0x29, "Haunted Lorelei 1"},
    {0x2A, "Haunted Lorelei 2"},      {0x2E, "Mila's Bar 1"},
    {0x2F, "Mila's Bar 2"},           {0x30, "Jann Lee/Leifang Train"},
    {0x31, "Helena's Ship 1"},        {0x32, "Mila's Shower/Closet"},
    {0x33, "Christie's Bar"},         {0x34, "Helena's Ship 2"},
    {0x35, "Oil Platform"},           {0x36, "Helena's Ship 3"},
    {0x37, "Helena's Ship 4"},        {0x38, "Kasumi's Secret Village"},
    {0x39, "Infiltrating Depth"},     {0x3A, "Christie's Bar 2"},
};

// Game mode IDs. Modes split by addr::variantFor are keyed as (mode << 8) | variant.
const std::unordered_map<uint32_t, const char*> kGameModes = {
    {0x00, "Story"},
    {0x01, "Versus"},
    {0x02, "Arcade"},        {0x0201, "Tag Arcade"},
    {0x03, "Time Attack"},
    {0x04, "Survival"},
    {0x05, "Training"},      {0x0500, "Free Training"}, {0x0501, "Command Training"},
    {0x08, "Online Match"},  // Ranked and Lobby matches share this ID
    {0x0F, "Watch Mode"},
    {0x10, "Team Fight"},
    {0x11, "Tutorial"},
    {0x12, "Combo Challenge"},
};

// 0-based difficulty levels. Tag Arcade numbers the same levels from 8.
const char* const kDifficulties[] = {
    "Rookie", "Easy", "Normal", "Hard", "Champ", "True Fighter", "Master", "Legend",
};

// Stage ID -> Discord asset key (files in resources/_discord_assets). Variants of a stage
// share one image; stages without an entry show the default large image.
const std::unordered_map<uint32_t, const char*> kStageImages = {
    {0x01, "stage_scramble"},  {0x02, "stage_scramble"},
    {0x03, "stage_fuel"},      {0x04, "stage_fuel"},
    {0x05, "stage_lab"},       {0x06, "stage_lab"},       {0x07, "stage_lab"},
    {0x08, "stage_flow"},      {0x09, "stage_flow"},
    {0x0A, "stage_sanctuary"},
    {0x0B, "stage_home"},      {0x0C, "stage_home"},
    {0x0D, "stage_street"},    {0x0E, "stage_street"},
    {0x0F, "stage_the_show"},
    {0x10, "stage_the_tiger_show"},
    {0x13, "stage_hotzone"},
    {0x14, "stage_the_ends_of_the_earth"},
    {0x16, "stage_depth"},
    {0x18, "stage_temple_of_the_dragon"},
    {0x19, "stage_sakura"},
    {0x1A, "stage_fighting_entertainment"},
    {0x1D, "stage_sweat"},     // Gym
    {0x1E, "stage_dead_or_alive"},
    {0x21, "stage_zack_island"},
    {0x22, "stage_forest"},
    {0x23, "stage_sky_city_tokyo"},
    {0x24, "stage_desert"},
    {0x25, "stage_lost_world"},
    {0x26, "stage_aircraft_carrier"},
    {0x29, "stage_haunted_lorelei"}, {0x2A, "stage_haunted_lorelei"},
};

const char* lookup(const std::unordered_map<uint32_t, const char*>& table, uint32_t id) {
    auto it = table.find(id);
    return it != table.end() ? it->second : nullptr;
}

} // namespace

const char* characterName(uint32_t id) { return lookup(kCharacters, id); }
const char* stageName(uint32_t id) { return lookup(kStages, id); }
const char* stageImage(uint32_t stage) { return lookup(kStageImages, stage); }

const char* gameModeName(uint32_t mode, std::optional<uint32_t> variant) {
    if (variant) {
        if (const char* name = lookup(kGameModes, (mode << 8) | (*variant & 0xFF))) return name;
    }
    return lookup(kGameModes, mode);
}

const char* difficultyName(uint32_t level) {
    constexpr uint32_t count = sizeof(kDifficulties) / sizeof(kDifficulties[0]);
    return level < 2 * count ? kDifficulties[level % count] : nullptr;
}

bool isSpectatorMode(uint32_t mode) { return mode == 0x0F; }

std::string assetKey(const std::string& name) {
    std::string key;
    for (unsigned char ch : name) {
        if (std::isalnum(ch)) {
            key += static_cast<char>(std::tolower(ch));
        } else if (ch == '\'') {
            continue; // "Mila's Bar" -> "milas_bar"
        } else if (!key.empty() && key.back() != '_') {
            key += '_';
        }
    }
    while (!key.empty() && key.back() == '_') key.pop_back();
    return key;
}
