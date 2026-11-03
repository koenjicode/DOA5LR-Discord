#pragma once

#include <string>

// User options from DOA5Discord.ini next to the .asi. Unlike settings.h, these are meant to be
// changed by players without rebuilding.
namespace config {

// Cover art used as the default large image.
// Plain names are the Western covers, "_jp" the Japanese ones.
enum class Cover
{
    Default,    // DOA5 Last Round
    DefaultJp,  // DOA5 Last Round, Japanese cover
    Ultimate,   // DOA5 Ultimate, Western character-grid cover
    UltimateJp, // DOA5 Ultimate, Japanese cover
    Vanilla,    // DOA5 (the 2012 base game)
};

struct Config
{
    // Cover art used as the default large image.
    Cover cover = Cover::Default;
    // Use large character icons for the characters instead of the small image icons.
    bool largeCharacterIcons = false;
};

// Reads <dir>\DOA5Discord.ini, creating it with the defaults first if it doesn't exist.
void load(const std::wstring& dir);

// The options read by load(); the defaults if load() hasn't run.
const Config& get();

} // namespace config
