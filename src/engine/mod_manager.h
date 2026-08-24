#pragma once

#include "mod_api.h"

#include <vector>
#include <string>

struct LoadedMod
{
    std::wstring fileName;
    HMODULE hModule = nullptr;

    // Automatically match function types straight from mod_api.h declarations
    using PfnGetInternalModPtr = decltype(&GetInternalModPtr);
    using PfnModInit = decltype(&ModInit);
    using PfnModAfterInit = decltype(&ModAfterInit);
    using PfnModBeforeUninit = decltype(&ModBeforeUninit);
    using PfnModUninit = decltype(&ModUninit);
    using PfnModSettingsChange = decltype(&ModSettingsChanged);

    PfnGetInternalModPtr GetInternalModPtr = nullptr;
    PfnModInit ModInit = nullptr;
    PfnModAfterInit ModAfterInit = nullptr;
    PfnModBeforeUninit ModBeforeUninit = nullptr;
    PfnModUninit ModUninit = nullptr;
    PfnModSettingsChange ModSettingsChanged = nullptr;
};

class ModManager
{
public:
    static ModManager& Instance()
    {
        static ModManager instance;
        return instance;
    }

    // Dynamic Loader & Unloader Core Methods
    bool LoadMod(const std::wstring& dllPath);
    void UnloadAllMods();

private:
    ModManager() = default;
    ~ModManager() { UnloadAllMods(); }

    std::vector<LoadedMod> m_loadedMods;
};
