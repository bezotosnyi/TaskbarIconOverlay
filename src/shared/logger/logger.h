#pragma once
// Thin wrapper around spdlog. Static methods, not an instance - there's
// only ever one log target per DLL that links this static library (see
// the .cpp for why "per DLL" matters here).
//
// Usage: call Init() once, early (DllMain DLL_PROCESS_ATTACH), then use
// Info/Warn/Error anywhere in that DLL.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>

class Logger
{
public:
    // ownModule: the calling DLL's own HMODULE (NOT the target/injected
    // module) - used to find the directory to log into, so the log file
    // ends up next to the binaries rather than a hardcoded system path.
    static void Init(HMODULE ownModule);

    static void Info(const std::wstring& text);
    static void Warn(const std::wstring& text);
    static void Error(const std::wstring& text);

    Logger() = delete;
};
