#include "logger.h"

#include "mod_manager.h"

namespace
{
    HMODULE g_hEngineModule = nullptr;

    std::wstring GetEngineDirectory()
    {
        wchar_t path[MAX_PATH];
        // Pass g_hEngineModule to get the path of engine.dll, NOT the host process (.exe)
        GetModuleFileNameW(g_hEngineModule, path, MAX_PATH);

        std::wstring fullPath(path);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos)
        {
            // Return path including the trailing slash (e.g., "C:\MyWrapperDir\")
            return fullPath.substr(0, lastSlash + 1);
        }
        return L"";
    }
}

static DWORD WINAPI InitializeEngineWorker(void* parameter)
{
    const auto engineModule = static_cast<HMODULE>(parameter);

    Logger::Init(engineModule);
    Logger::Info(L"=== Engine worker, PID=" + std::to_wstring(GetCurrentProcessId()) + L" ===");

    std::wstring engineDir = GetEngineDirectory();
    auto modManager = &ModManager::Instance();
    modManager->LoadMod(engineDir + L"taskbar-grouping.dll");
    modManager->LoadMod(engineDir + L"taskbar-icon-overlay.dll");

    return 0;
}

// Diagnostic-only export, confirmed via dumpbin /exports back when we
// first verified vcpkg linked correctly. Harmless to keep.
extern "C" __declspec(dllexport) BOOL WINAPI PingEngine()
{
    return TRUE;
}

// Called remotely by the injector (CreateRemoteThread + RVA lookup, see
// CallRemoteExport in TaskbarIconOverlay.Injector/main.cpp) for a clean
// unload without restarting explorer.exe. No hooks exist yet in this
// minimal version, so there's nothing to unhook - this just logs and
// frees the module.
extern "C" __declspec(dllexport) DWORD WINAPI EngineShutdown(LPVOID)
{
    Logger::Info(L"=== EngineShutdown called ===");

    ModManager::Instance().UnloadAllMods();

    HMODULE hSelf = nullptr;
    // GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT is required here -
    // without it, GetModuleHandleExW increments the module's refcount by
    // one (same as LoadLibrary), and FreeLibraryAndExitThread below only
    // removes ONE reference, leaving the DLL loaded forever (this exact
    // bug was hit and fixed earlier).
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&EngineShutdown), &hSelf);

    // Does not return.
    FreeLibraryAndExitThread(hSelf, 0);
}

BOOL APIENTRY DllMain(HMODULE engineModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hEngineModule = engineModule;

        DisableThreadLibraryCalls(engineModule);

        auto worker = CreateThread(
            nullptr, 0, InitializeEngineWorker, engineModule, 0, nullptr);
        if (worker) CloseHandle(worker);

        return TRUE;
    }

    return TRUE;
}
