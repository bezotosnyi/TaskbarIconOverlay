#include "mod_manager.h"

#include "logger.h"

bool ModManager::LoadMod(const std::wstring& dllPath)
{
    LoadedMod mod;

    // Extract filename from the target path for clean logging metrics
    size_t lastSlash = dllPath.find_last_of(L"\\/");
    mod.fileName = (lastSlash != std::wstring::npos) ? dllPath.substr(lastSlash + 1) : dllPath;

    // Map the target module directly into our current process workspace
    mod.hModule = LoadLibraryW(dllPath.c_str());
    if (!mod.hModule)
    {
        Logger::Error(L"Failed to map file: " + dllPath);
        return false;
    }

    // Safely resolve exported endpoints using type-casting protection variables
    mod.GetInternalModPtr = reinterpret_cast<LoadedMod::PfnGetInternalModPtr>(
        GetProcAddress(mod.hModule, "GetInternalModPtr")
        );
    mod.ModInit = reinterpret_cast<LoadedMod::PfnModInit>(
        GetProcAddress(mod.hModule, "ModInit")
    );
    mod.ModAfterInit = reinterpret_cast<LoadedMod::PfnModAfterInit>(
        GetProcAddress(mod.hModule, "ModAfterInit")
    );
    mod.ModBeforeUninit = reinterpret_cast<LoadedMod::PfnModBeforeUninit>(
        GetProcAddress(mod.hModule, "ModBeforeUninit")
    );
    mod.ModUninit = reinterpret_cast<LoadedMod::PfnModUninit>(
        GetProcAddress(mod.hModule, "ModUninit")
    );
    mod.ModSettingsChanged = reinterpret_cast<LoadedMod::PfnModSettingsChange>(
        GetProcAddress(mod.hModule, "ModSettingsChanged")
    );

    // Guard against malformed files that don't match our contract architecture
    if (!mod.ModInit)
    {
        Logger::Error(L"Mod missing required ModInit export: " + mod.fileName);
        FreeLibrary(mod.hModule);
        return false;
    }

    // Fire standard contract execution paths in accordance with Windhawk specs
    Logger::Info(L"Initializing mod: " + mod.fileName);
    if (mod.ModInit())
    {
        if (mod.ModAfterInit)
        {
            mod.ModAfterInit();
        }

        // Track valid operating mod references safely
        m_loadedMods.push_back(mod);
        return true;
    }

    // Drop library linkage if initialization parameters explicitly failed
    Logger::Error(L"ModInit returned FALSE for: " + mod.fileName);
    FreeLibrary(mod.hModule);
    return false;
}

void ModManager::UnloadAllMods()
{
    // Traverse container backward (FILO Stack layout) to process drops without memory corruption
    for (auto it = m_loadedMods.rbegin(); it != m_loadedMods.rend(); ++it)
    {
        if (!it->hModule)
        {
            continue;
        }

        Logger::Info(L"Tearing down mod context: " + it->fileName);

        if (it->ModBeforeUninit)
        {
            it->ModBeforeUninit();
            Sleep(50);
        }

        // Trigger target's private cleanup lifecycles first
        if (it->ModUninit)
        {
            it->ModUninit();
            Sleep(50);
        }

        // Unmap module context. This automatically fires target DllMain's DLL_PROCESS_DETACH,
        // which dynamically strips out MinHook hooks completely in absolute safety.
        FreeLibrary(it->hModule);
    }

    m_loadedMods.clear();
}
