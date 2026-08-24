#pragma once

#include "windhawk_api.h"
#include "logger.h"

#include <MinHook.h>
#include <string>

// Forward declaration of the specific function each mod must write
void InitializeModDefaultSettings();

inline std::wstring GetCurrentModuleName(HMODULE hModule)
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hModule, path, MAX_PATH);

    std::wstring fullPath(path);
    auto lastSlash = fullPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
    {
        return fullPath.substr(lastSlash + 1);
    }

    return fullPath;
}

// The complete lifecycle entry point injected directly into the mod
BOOL APIENTRY DllMain(HMODULE modeModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        InternalWhModPtr = static_cast<void*>(modeModule);

        Logger::Init(modeModule);
        std::wstring moduleName = GetCurrentModuleName(modeModule);
        Logger::Info(L"=== Mod '" + moduleName + L"' attached ===");

        DisableThreadLibraryCalls(modeModule);

        // Run the specific mod's custom defaults seeding
        InitializeModDefaultSettings();
        return TRUE;
    }

    if (reason == DLL_PROCESS_DETACH)
    {
        std::wstring moduleName = GetCurrentModuleName(modeModule);
        Logger::Info(L"=== Mod '" + moduleName + L"' detaching ===");

        MH_DisableHook(nullptr);
        MH_Uninitialize();

        Logger::Info(L"Hooks removed, unloading '" + moduleName + L"' module structure.");
    }
    return TRUE;
}
