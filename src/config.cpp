#include "config.h"
#include "log.h"
#include "settings.h"

#include <Windows.h>
#include <cwctype>
#include <iterator>

namespace config {

namespace {

// Written as DOA5Discord.ini when there isn't one, so it doubles as the documentation.
constexpr char kDefaultIni[] =
    "[general]\r\n"
    "; The cover art that is used as the default large image\r\n"
    "; Available options: (default, default_jp, ultimate, ultimate_jp, vanilla)\r\n"
    "Cover=default\r\n"
    "\r\n"
    "; Show your character's large art as the large image in battles\r\n"
    "; instead of the stage or cover art: (1 = yes, 0 = no)\r\n"
    "LargeCharacterIcons=0\r\n"
    ;

Config g_config;
std::wstring g_path;

bool fileExists(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

void writeDefaults(const std::wstring& path)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(file, kDefaultIni, sizeof(kDefaultIni) - 1, &written, nullptr);
    CloseHandle(file);
}

std::wstring readString(const wchar_t* section, const wchar_t* key, const wchar_t* fallback)
{
    wchar_t buffer[256];
    GetPrivateProfileStringW(section, key, fallback, buffer, static_cast<DWORD>(std::size(buffer)), g_path.c_str());
    return buffer;
}

[[maybe_unused]] int readInt(const wchar_t* section, const wchar_t* key, int fallback)
{
    return static_cast<int>(GetPrivateProfileIntW(section, key, fallback, g_path.c_str()));
}

// Accepts 1/0, true/false, yes/no and on/off, in any case.
bool readBool(const wchar_t* section, const wchar_t* key, bool fallback)
{
    std::wstring value = readString(section, key, L"");
    for (wchar_t& ch : value) ch = static_cast<wchar_t>(std::towlower(ch));
    if (value == L"1" || value == L"true" || value == L"yes" || value == L"on") return true;
    if (value == L"0" || value == L"false" || value == L"no" || value == L"off") return false;
    return fallback;
}

// Accepts the names written in kDefaultIni, ignoring case, spaces, '_' and '-'
// ("Default JP", "ultimate-jp", ...). The earlier names LastRoundJP, UltimateGrid and Original still work.
Cover readCover(const wchar_t* section, const wchar_t* key, Cover fallback)
{
    std::wstring value;
    for (wchar_t ch : readString(section, key, L""))
        if (ch != L' ' && ch != L'_' && ch != L'-') value += static_cast<wchar_t>(std::towlower(ch));
    if (value == L"default") return Cover::Default;
    if (value == L"defaultjp" || value == L"lastroundjp") return Cover::DefaultJp;
    if (value == L"ultimate" || value == L"ultimategrid") return Cover::Ultimate;
    if (value == L"ultimatejp") return Cover::UltimateJp;
    if (value == L"vanilla" || value == L"original") return Cover::Vanilla;
    return fallback;
}

const char* coverName(Cover cover)
{
    switch (cover)
    {
    case Cover::DefaultJp: return "default_jp";
    case Cover::Ultimate: return "ultimate";
    case Cover::UltimateJp: return "ultimate_jp";
    case Cover::Vanilla: return "vanilla";
    default: return "default";
    }
}

} // namespace

void load(const std::wstring& dir)
{
    if (!settings::kEnableConfig)
    {
        g_config = Config{};
        LOG("config: disabled, using defaults");
        return;
    }

    g_path = dir + L"\\DOA5Discord.ini";
    if (!fileExists(g_path)) writeDefaults(g_path);

    Config c;
    c.cover = readCover(L"general", L"Cover", c.cover);
    c.largeCharacterIcons = readBool(L"general", L"LargeCharacterIcons", c.largeCharacterIcons);
    g_config = c;

    LOG("config: Cover=%s, LargeCharacterIcons=%d", coverName(g_config.cover), g_config.largeCharacterIcons);
}

const Config& get()
{
    return g_config;
}

} // namespace config
