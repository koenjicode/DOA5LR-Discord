#pragma once

#include <string>

namespace logger {

// Opens <dir>\DOA5Discord.log. Logging is a no-op until this is called.
void open(const std::wstring& dir);
void write(const char* fmt, ...);

} // namespace logger

#define LOG(...) ::logger::write(__VA_ARGS__)
