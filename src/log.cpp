#include "log.h"

#include <Windows.h>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <share.h>

namespace logger {

static FILE* g_file = nullptr;
static std::mutex g_mutex;

void open(const std::wstring& dir) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file) return;
    std::wstring path = dir + L"\\DOA5Discord.log";
    // Shared so the log can be read while the game is running.
    g_file = _wfsopen(path.c_str(), L"w", _SH_DENYNO);
}

void write(const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_file) return;

    SYSTEMTIME t;
    GetLocalTime(&t);
    fprintf(g_file, "[%02d:%02d:%02d.%03d] ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

    va_list args;
    va_start(args, fmt);
    vfprintf(g_file, fmt, args);
    va_end(args);

    fputc('\n', g_file);
    fflush(g_file);
}

} // namespace logger
