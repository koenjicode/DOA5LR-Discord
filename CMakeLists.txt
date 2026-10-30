cmake_minimum_required(VERSION 3.20)
project(DOA5Discord LANGUAGES CXX)

# DOA5LR's game.exe is a 32-bit process, so the plugin must be 32-bit too.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "game.exe is 32-bit. Configure with: cmake -A Win32 (or use the 'win32' preset).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static CRT so users don't need the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(DISCORD_SDK_DIR ${CMAKE_SOURCE_DIR}/third_party/discord_game_sdk)
if(NOT EXISTS ${DISCORD_SDK_DIR}/c/discord_game_sdk.h)
    message(FATAL_ERROR "Discord Game SDK not found. Extract it into third_party/discord_game_sdk (see README).")
endif()

# Headers are listed so IDEs (Rider, Visual Studio) show them as part of the project.
add_library(DOA5Discord SHARED
    src/main.cpp
    src/addresses.cpp
    src/addresses.h
    src/game.cpp
    src/game.h
    src/names.cpp
    src/names.h
    src/discord.cpp
    src/discord.h
    src/log.cpp
    src/log.h
    src/settings.h
)

set_target_properties(DOA5Discord PROPERTIES SUFFIX ".asi")
target_include_directories(DOA5Discord PRIVATE ${DISCORD_SDK_DIR}/c)
target_compile_definitions(DOA5Discord PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_compile_options(DOA5Discord PRIVATE /W4 /utf-8)

# The SDK DLL is loaded at runtime with LoadLibrary, so no import library is linked.
# Copy everything a user needs into the output folder.
add_custom_command(TARGET DOA5Discord POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DISCORD_SDK_DIR}/lib/x86/discord_game_sdk.dll $<TARGET_FILE_DIR:DOA5Discord>
)
