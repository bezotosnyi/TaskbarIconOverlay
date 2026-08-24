#include "dia_symbol_resolver.h"

#include "logger.h"

#include <dbghelp.h>
#include <dia2.h>
#include <oleauto.h>
#include <shlobj.h>

#include <cwchar>
#include <string>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shell32.lib")

namespace
{
    // --- Names of our own redistributed binaries (see redist/README.md) ---
    constexpr wchar_t kDiaDllName[] = L"TaskbarIconOverlay.Dia.dll";
    constexpr wchar_t kSymSrvDllName[] = L"TaskbarIconOverlay.SymSrv.dll";

    // --- Names DIA's own private msdia/symsrv pair looks for internally -
    // these are NOT our files, they're what we intercept and redirect. ---
    constexpr wchar_t kRealSymSrvDllName[] = L"SYMSRV.DLL";
    constexpr char kKernel32DllName[] = "KERNEL32.dll";
    constexpr char kLoadLibraryExWName[] = "LoadLibraryExW";
    constexpr char kDllGetClassObjectName[] = "DllGetClassObject";

    constexpr wchar_t kSymbolCacheSubdir[] = L"TaskbarIconOverlay\\symbols";
    constexpr wchar_t kSymbolServerUrl[] = L"https://msdl.microsoft.com/download/symbols";

    std::wstring g_symSrvPath;

    std::wstring ModuleDirectory(HMODULE module)
    {
        wchar_t path[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(module, path, ARRAYSIZE(path));
        if (!length || length == ARRAYSIZE(path))
            return {};

        std::wstring result(path, length);
        const size_t slash = result.find_last_of(L"\\/");
        return slash == std::wstring::npos ? std::wstring{} : result.substr(0, slash + 1);
    }

    // %LOCALAPPDATA%\TaskbarIconOverlay\symbols - not a hardcoded C:\ path,
    // since not every machine has C: as the system drive or write access to
    // its root.
    std::wstring SymbolCacheDirectory()
    {
        PWSTR localAppData = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData)))
        {
            return L"C:\\TaskbarIconOverlay\\symbols"; // last-resort fallback
        }
        std::wstring result = std::wstring(localAppData) + L"\\" + kSymbolCacheSubdir;
        CoTaskMemFree(localAppData);
        return result;
    }

    void LogLastError(const wchar_t* operation)
    {
        Logger::Warn(std::wstring(operation) + L" failed, error=" + std::to_wstring(GetLastError()));
    }

    void LogHr(const wchar_t* operation, HRESULT hr)
    {
        wchar_t value[16]{};
        swprintf_s(value, L"%08X", static_cast<unsigned int>(hr));
        Logger::Warn(std::wstring(operation) + L" failed, HRESULT=0x" + value);
    }

    void** FindImportSlot(HMODULE module, const char* importedDll, const char* importedName)
    {
        auto* image = reinterpret_cast<unsigned char*>(module);
        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return nullptr;

        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return nullptr;

        const IMAGE_DATA_DIRECTORY& directory =
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (!directory.VirtualAddress)
            return nullptr;

        auto* descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(image + directory.VirtualAddress);
        for (; descriptor->Name; ++descriptor)
        {
            auto dllName = reinterpret_cast<const char*>(image + descriptor->Name);
            if (_stricmp(dllName, importedDll) != 0)
                continue;

            const DWORD lookupRva = descriptor->OriginalFirstThunk
                                        ? descriptor->OriginalFirstThunk
                                        : descriptor->FirstThunk;
            auto* lookup = reinterpret_cast<IMAGE_THUNK_DATA64*>(image + lookupRva);
            auto* iat = reinterpret_cast<IMAGE_THUNK_DATA64*>(image + descriptor->FirstThunk);

            for (; lookup->u1.AddressOfData; ++lookup, ++iat)
            {
                if (IMAGE_SNAP_BY_ORDINAL64(lookup->u1.Ordinal))
                {
                    continue;
                }

                auto* byName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
                    image + lookup->u1.AddressOfData);
                if (strcmp(byName->Name, importedName) == 0)
                {
                    return reinterpret_cast<void**>(&iat->u1.Function);
                }
            }
        }

        return nullptr;
    }

    HMODULE WINAPI RedirectSymSrvLoad(LPCWSTR fileName, HANDLE file, DWORD flags)
    {
        if (!fileName || _wcsicmp(fileName, kRealSymSrvDllName) != 0)
        {
            return LoadLibraryExW(fileName, file, flags);
        }

        DWORD redirectedFlags = flags | LOAD_WITH_ALTERED_SEARCH_PATH;
        redirectedFlags &= ~LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR;
        redirectedFlags &= ~LOAD_LIBRARY_SEARCH_APPLICATION_DIR;
        redirectedFlags &= ~LOAD_LIBRARY_SEARCH_USER_DIRS;
        redirectedFlags &= ~LOAD_LIBRARY_SEARCH_SYSTEM32;
        redirectedFlags &= ~LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;

        return LoadLibraryExW(g_symSrvPath.c_str(), file, redirectedFlags);
    }

    HRESULT CreateDiaSource(HMODULE diaModule, IDiaDataSource** source)
    {
        using DllGetClassObject_t = HRESULT(STDAPICALLTYPE*)(REFCLSID, REFIID, void**);
        auto getClassObject = reinterpret_cast<DllGetClassObject_t>(
            GetProcAddress(diaModule, kDllGetClassObjectName));
        if (!getClassObject)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        IClassFactory* factory = nullptr;
        HRESULT hr = getClassObject(CLSID_DiaSource, IID_IClassFactory,
                                    reinterpret_cast<void**>(&factory));
        if (FAILED(hr))
        {
            return hr;
        }

        hr = factory->CreateInstance(nullptr, IID_IDiaDataSource,
                                     reinterpret_cast<void**>(source));
        factory->Release();
        return hr;
    }

    bool PatchMsdiaSymSrvImport(HMODULE diaModule)
    {
        void** slot = FindImportSlot(diaModule, kKernel32DllName, kLoadLibraryExWName);
        if (!slot)
        {
            Logger::Warn(L"DIA LoadLibraryExW import was not found");
            return false;
        }

        DWORD oldProtection{};
        if (!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &oldProtection))
        {
            LogLastError(L"VirtualProtect(DIA IAT)");
            return false;
        }

        *slot = reinterpret_cast<void*>(&RedirectSymSrvLoad);
        DWORD ignored{};
        VirtualProtect(slot, sizeof(*slot), oldProtection, &ignored);
        return true;
    }

    // RAII wrapper around a DIA session: the constructor does all the setup
    // shared by both FindSymbol and FindSymbols (load the private DIA DLL,
    // patch its SymSrv import, create the data source, load the PDB for
    // targetModule, open a session, grab the global scope). The destructor
    // releases whatever got created, however far setup got before failing -
    // no manually matched Release()/FreeLibrary calls on every exit branch.
    class DiaSession
    {
    public:
        DiaSession(HMODULE engineModule, HMODULE targetModule, bool verbose)
        {
            if (verbose) Logger::Info(L"DiaSession: opening");

            const std::wstring engineDirectory = ModuleDirectory(engineModule);
            if (engineDirectory.empty())
            {
                Logger::Warn(L"Could not determine the caller's own module path");
                return;
            }

            const std::wstring diaPath = engineDirectory + kDiaDllName;
            g_symSrvPath = engineDirectory + kSymSrvDllName;

            m_diaModule = LoadLibraryExW(diaPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (!m_diaModule)
            {
                LogLastError(L"LoadLibraryExW(TaskbarIconOverlay.Dia.dll)");
                return;
            }

            if (!PatchMsdiaSymSrvImport(m_diaModule))
                return;

            IDiaDataSource* source = nullptr;
            HRESULT hr = CreateDiaSource(m_diaModule, &source);
            if (FAILED(hr))
            {
                LogHr(L"CreateDiaSource", hr);
                return;
            }

            wchar_t imagePath[MAX_PATH]{};
            if (!GetModuleFileNameW(targetModule, imagePath, ARRAYSIZE(imagePath)))
            {
                LogLastError(L"GetModuleFileNameW(target)");
                source->Release();
                return;
            }

            const std::wstring symbolPath =
                L"srv*" + SymbolCacheDirectory() + L"*" + kSymbolServerUrl;
            hr = source->loadDataForExe(imagePath, symbolPath.c_str(), nullptr);
            if (FAILED(hr))
            {
                LogHr(L"IDiaDataSource::loadDataForExe", hr);
                source->Release();
                return;
            }

            hr = source->openSession(&m_session);
            if (SUCCEEDED(hr))
                hr = m_session->get_globalScope(&m_global);
            source->Release(); // not needed past openSession

            if (FAILED(hr))
            {
                LogHr(L"DIA openSession/get_globalScope", hr);
                return;
            }

            if (verbose) Logger::Info(L"DiaSession: ready");
        }

        ~DiaSession()
        {
            if (m_global) m_global->Release();
            if (m_session) m_session->Release();
            if (m_diaModule) FreeLibrary(m_diaModule);
        }

        DiaSession(const DiaSession&) = delete;
        DiaSession& operator=(const DiaSession&) = delete;

        bool IsValid() const { return m_global != nullptr; }
        IDiaSymbol* Global() const { return m_global; }

    private:
        HMODULE m_diaModule = nullptr;
        IDiaSession* m_session = nullptr;
        IDiaSymbol* m_global = nullptr;
    };
} // namespace

DiaSymbolResolver::DiaSymbolResolver(bool verboseTrace) : m_verbose(verboseTrace)
{
}

std::unordered_map<std::wstring, void*> DiaSymbolResolver::FindSymbols(
    HMODULE engineModule, HMODULE targetModule,
    const std::vector<std::wstring>& undecoratedNames)
{
    std::unordered_map<std::wstring, void*> results;

    // Fast lookup set for matching inside the enumeration loop below. Names
    // already in this instance's cache are resolved immediately and
    // removed from pending - they never trigger a DIA session.
    std::unordered_map<std::wstring, bool> pending;
    auto& moduleCache = m_cache[targetModule]; // creates an empty entry on first use

    for (const auto& name : undecoratedNames)
    {
        auto cached = moduleCache.find(name);
        if (cached != moduleCache.end())
        {
            results[name] = cached->second;
        }
        else
        {
            pending[name] = true;
        }
    }

    if (m_verbose)
    {
        Logger::Info(L"FindSymbols entered, targets=" + std::to_wstring(undecoratedNames.size()) +
            L", cache hits=" + std::to_wstring(results.size()));
    }

    if (pending.empty())
    {
        // Everything requested was already cached - no DIA session needed.
        if (m_verbose) Logger::Info(L"FindSymbols done (all from cache)");
        return results;
    }

    DiaSession dia(engineModule, targetModule, m_verbose);
    if (!dia.IsValid())
    {
        return results; // failure already logged inside DiaSession's constructor
    }

    constexpr enum SymTagEnum tags[] = {SymTagPublicSymbol, SymTagFunction, SymTagData};
    for (enum SymTagEnum tag : tags)
    {
        if (pending.empty()) break; // everything already found - skip remaining tags

        IDiaEnumSymbols* symbols = nullptr;
        HRESULT hr = dia.Global()->findChildren(tag, nullptr, nsNone, &symbols);
        if (FAILED(hr)) continue;

        for (;;)
        {
            if (pending.empty()) break;

            IDiaSymbol* symbol = nullptr;
            ULONG fetched{};
            hr = symbols->Next(1, &symbol, &fetched);
            if (hr != S_OK || !fetched) break;

            BSTR name = nullptr;
            const HRESULT nameHr = symbol->get_undecoratedName(&name);
            if (nameHr == S_OK && name)
            {
                auto it = pending.find(name);
                if (it != pending.end())
                {
                    DWORD rva{};
                    if (symbol->get_relativeVirtualAddress(&rva) == S_OK)
                    {
                        void* address = reinterpret_cast<BYTE*>(targetModule) + rva;
                        results[it->first] = address;
                        moduleCache[it->first] = address; // cache for future calls
                        if (m_verbose)
                        {
                            Logger::Info(L"FindSymbols matched: [" + it->first + L"], RVA=" +
                                std::to_wstring(rva));
                        }
                    }
                    pending.erase(it);
                }
            }

            if (name) SysFreeString(name);
            symbol->Release();
        }

        symbols->Release();
    }

    for (const auto& [name, stillPending] : pending)
    {
        Logger::Warn(L"DiaSymbolResolver: symbol not found: " + name);
    }

    if (m_verbose)
    {
        Logger::Info(L"FindSymbols done: " + std::to_wstring(results.size()) + L"/" +
            std::to_wstring(undecoratedNames.size()) + L" resolved");
    }

    return results;
}

void* DiaSymbolResolver::FindSymbol(HMODULE engineModule, HMODULE targetModule,
                                    const wchar_t* undecoratedName)
{
    auto results = FindSymbols(engineModule, targetModule, {undecoratedName});
    auto it = results.find(undecoratedName);
    return it != results.end() ? it->second : nullptr;
}
