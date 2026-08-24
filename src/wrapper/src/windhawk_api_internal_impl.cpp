#include "windhawk_api_internal.h"

#include "dia_symbol_resolver.h"
#include "logger.h"

#include <MinHook.h>
#include <shlobj.h>

#include <cassert>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#pragma comment(lib, "shell32.lib")

namespace
{
    // Non-throwing on purpose: these functions can end up called from deep
    // inside explorer.exe's own call stack (mod lifecycle callbacks, hook
    // functions). An uncaught C++ exception crossing an extern "C" boundary
    // there is undefined behavior - a genuinely broken crash, not a clean one.
    // assert() gives the same "you'll notice this in Debug" signal without
    // that risk, and is a no-op in Release rather than taking the whole
    // process down.
#define NOT_IMPLEMENTED()                                       \
    do {                                                        \
        OutputDebugStringA("NOT IMPLEMENTED: " __FUNCTION__);   \
        assert(false && "NOT_IMPLEMENTED reached");             \
    } while (0)

    class SettingsRegistry
    {
    public:
        using SettingValue = std::variant<int, std::wstring, std::vector<BYTE>>;

        static SettingsRegistry& Instance()
        {
            static SettingsRegistry instance;
            return instance;
        }

        void SetInt(const std::wstring& key, int value) { m_storage[key] = value; }
        void SetString(const std::wstring& key, const std::wstring& value) { m_storage[key] = value; }

        void SetBinary(const std::wstring& key, const void* data, size_t size)
        {
            if (data && size > 0)
            {
                auto byteData = static_cast<const BYTE*>(data);
                m_storage[key] = std::vector<BYTE>(byteData, byteData + size);
            }
            else
            {
                m_storage[key] = std::vector<BYTE>();
            }
        }

        bool GetInt(const std::wstring& key, int& outValue) const
        {
            auto it = m_storage.find(key);
            if (it != m_storage.end() && std::holds_alternative<int>(it->second))
            {
                outValue = std::get<int>(it->second);
                return true;
            }
            return false;
        }

        const std::wstring* GetString(const std::wstring& key) const
        {
            auto it = m_storage.find(key);
            if (it != m_storage.end() && std::holds_alternative<std::wstring>(it->second))
            {
                return &std::get<std::wstring>(it->second);
            }
            return nullptr;
        }

        const std::vector<BYTE>* GetBinary(const std::wstring& key) const
        {
            auto it = m_storage.find(key);
            if (it != m_storage.end() && std::holds_alternative<std::vector<BYTE>>(it->second))
            {
                return &std::get<std::vector<BYTE>>(it->second);
            }
            return nullptr;
        }

        bool Delete(const std::wstring& key) { return m_storage.erase(key) > 0; }

    private:
        SettingsRegistry() = default;
        std::unordered_map<std::wstring, SettingValue> m_storage;
    };

    void EnsureMinHookInitialized()
    {
        static bool initialized = false;
        if (initialized) return;

        MH_STATUS status = MH_Initialize();
        if (status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED)
        {
            initialized = true;
        }
        else
        {
            Logger::Error(L"MH_Initialize failed, status=" + std::to_wstring(status));
        }
    }

    // One resolver instance per DLL (function-local static, lazily
    // constructed) so its per-module symbol cache actually pays off across
    // repeated InternalWh_HookSymbols calls within that DLL's lifetime.
    DiaSymbolResolver& GetResolver()
    {
        static DiaSymbolResolver resolver(true);
        return resolver;
    }
} // namespace

// --- Logging ---

BOOL InternalWh_IsLogEnabled(void*)
{
#ifdef _DEBUG
    return TRUE;
#else
    return FALSE;
#endif
}

void InternalWh_Log(void*, PCWSTR format, va_list args)
{
    wchar_t buffer[2048];
    vswprintf_s(buffer, format, args);
    Logger::Info(buffer);
}

// --- Mod values (Wh_Get/SetIntValue etc.) ---

int InternalWh_GetIntValue(void*, PCWSTR valueName, int defaultValue)
{
    if (!valueName) return defaultValue;
    int value = 0;
    if (SettingsRegistry::Instance().GetInt(valueName, value))
    {
        return value;
    }
    return defaultValue;
}

BOOL InternalWh_SetIntValue(void*, PCWSTR valueName, int value)
{
    if (!valueName) return FALSE;
    SettingsRegistry::Instance().SetInt(valueName, value);
    return TRUE;
}

size_t InternalWh_GetStringValue(void*, PCWSTR valueName, PWSTR stringBuffer, size_t bufferChars)
{
    if (!valueName) return 0;

    const std::wstring* str = SettingsRegistry::Instance().GetString(valueName);
    if (!str) return 0;

    size_t requiredChars = str->length() + 1;

    if (!stringBuffer || bufferChars == 0)
    {
        return requiredChars;
    }

    size_t copyChars = (requiredChars < bufferChars) ? str->length() : (bufferChars - 1);
    wcsncpy_s(stringBuffer, bufferChars, str->c_str(), copyChars);

    return requiredChars;
}

BOOL InternalWh_SetStringValue(void*, PCWSTR valueName, PCWSTR value)
{
    if (!valueName || !value) return FALSE;
    SettingsRegistry::Instance().SetString(valueName, value);
    return TRUE;
}

size_t InternalWh_GetBinaryValue(void*, PCWSTR valueName, void* buffer, size_t bufferSize)
{
    if (!valueName) return 0;

    const std::vector<BYTE>* vec = SettingsRegistry::Instance().GetBinary(valueName);
    if (!vec) return 0;

    size_t requiredSize = vec->size();

    if (!buffer || bufferSize == 0)
    {
        return requiredSize;
    }

    size_t copySize = (requiredSize < bufferSize) ? requiredSize : bufferSize;
    if (copySize > 0)
    {
        memcpy(buffer, vec->data(), copySize);
    }

    return requiredSize;
}

BOOL InternalWh_SetBinaryValue(void*, PCWSTR valueName, const void* buffer, size_t bufferSize)
{
    if (!valueName) return FALSE;
    SettingsRegistry::Instance().SetBinary(valueName, buffer, bufferSize);
    return TRUE;
}

BOOL InternalWh_DeleteValue(void*, PCWSTR valueName)
{
    if (!valueName) return FALSE;
    return SettingsRegistry::Instance().Delete(valueName) ? TRUE : FALSE;
}

// --- Mod storage path ---
//
// Neither taskbar-grouping nor taskbar-thumbnail-reorder actually call
// Wh_GetModStoragePath (confirmed earlier via grep over both sources), so
// this is currently unexercised - implemented anyway since it's cheap and
// a future mod (numbering) might want it. Not per-mod-identity (we ignore
// the `mod` parameter throughout, per the earlier decision that per-DLL
// static linking already gives isolation) - every caller gets the same
// shared directory.
size_t InternalWh_GetModStoragePath(void*, PWSTR pathBuffer, size_t bufferChars)
{
    PWSTR localAppData = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData)))
    {
        return 0;
    }
    std::wstring path = std::wstring(localAppData) + L"\\TaskbarIconOverlay\\ModStorage";
    CoTaskMemFree(localAppData);

    SHCreateDirectoryExW(nullptr, path.c_str(), nullptr); // recursive mkdir, ignores "already exists"

    if (!pathBuffer || bufferChars == 0)
    {
        return path.length() + 1;
    }

    size_t copyChars = (path.length() < bufferChars) ? path.length() : (bufferChars - 1);
    wcsncpy_s(pathBuffer, bufferChars, path.c_str(), copyChars);

    return path.length() + 1;
}

// --- Settings (Wh_Get*Setting) ---
//
// NOTE: `args` is ignored - correct only as long as neither mod uses
// indexed/printf-style setting names (e.g. L"items[%d].value"). Confirmed
// safe for the exact Wh_Get*Setting call sites already grepped in both
// taskbar-grouping and taskbar-thumbnail-reorder, but re-check this if a
// future mod's settings usage looks different.

int InternalWh_GetIntSetting(void*, PCWSTR valueName, va_list)
{
    if (!valueName) return 0;
    int value = 0;
    if (SettingsRegistry::Instance().GetInt(valueName, value))
    {
        return value;
    }
    return 0;
}

PCWSTR InternalWh_GetStringSetting(void*, PCWSTR valueName, va_list)
{
    if (!valueName) return L"";

    const std::wstring* str = SettingsRegistry::Instance().GetString(valueName);
    if (!str) return L"";

    size_t sizeInBytes = (str->length() + 1) * sizeof(wchar_t);
    auto buffer = static_cast<wchar_t*>(HeapAlloc(GetProcessHeap(), 0, sizeInBytes));

    if (buffer)
    {
        wcscpy_s(buffer, str->length() + 1, str->c_str());
    }
    return buffer;
}

void InternalWh_FreeStringSetting(void*, PCWSTR string)
{
    if (string && string != L"")
    {
        HeapFree(GetProcessHeap(), 0, const_cast<PWSTR>(string));
    }
}

// --- Hooks ---

BOOL InternalWh_SetFunctionHook(void*, void* targetFunction, void* hookFunction,
                                void** originalFunction)
{
    EnsureMinHookInitialized();

    auto createStatus = MH_CreateHook(targetFunction, hookFunction, originalFunction);
    if (createStatus != MH_OK && createStatus != MH_ERROR_ALREADY_CREATED)
    {
        Logger::Warn(L"MH_CreateHook failed, status=" + std::to_wstring(createStatus));
        return FALSE;
    }

    auto enableStatus = MH_EnableHook(targetFunction);
    if (enableStatus != MH_OK)
    {
        Logger::Warn(L"MH_EnableHook failed, status=" + std::to_wstring(enableStatus));
        return FALSE;
    }

    return TRUE;
}

BOOL InternalWh_RemoveFunctionHook(void*, void* targetFunction)
{
    MH_DisableHook(targetFunction);
    return MH_RemoveHook(targetFunction) == MH_OK;
}

BOOL InternalWh_ApplyHookOperations(void*)
{
    // MinHook applies each hook immediately inside InternalWh_SetFunctionHook
    // above (MH_EnableHook is called right there) - nothing left to do here.
    return TRUE;
}

// --- Symbol-based hooking ---
//
// WindhawkUtils::HookSymbols (header-only, in windhawk_utils.h) calls
// straight into this single function - there's no separate
// FindFirstSymbol/FindNextSymbol/FindCloseSymbol enumerator protocol to
// implement, confirmed from the real windhawk_utils.h source. Those three
// remain NOT_IMPLEMENTED below since neither mod calls them directly.

BOOL InternalWh_HookSymbols(void* mod, HMODULE module, const WH_SYMBOL_HOOK* symbolHooks,
                            size_t symbolHooksCount, const WH_HOOK_SYMBOLS_OPTIONS*)
{
    // Each entry lists one or more candidate names (first match wins) -
    // collect every candidate from every entry into one batched
    // DiaSymbolResolver::FindSymbols call instead of resolving one at a
    // time.
    std::vector<std::wstring> allCandidates;
    for (size_t i = 0; i < symbolHooksCount; i++)
    {
        const auto& entry = symbolHooks[i];
        for (size_t j = 0; j < entry.symbolsCount; j++)
        {
            allCandidates.emplace_back(entry.symbols[j].string, entry.symbols[j].length);
        }
    }

    auto ownModule = static_cast<HMODULE>(InternalWhModPtr);
    auto resolved = GetResolver().FindSymbols(ownModule, module, allCandidates);

    bool allRequiredOk = true;

    for (size_t i = 0; i < symbolHooksCount; i++)
    {
        const WH_SYMBOL_HOOK& entry = symbolHooks[i];

        void* address = nullptr;
        for (size_t j = 0; j < entry.symbolsCount; j++)
        {
            std::wstring candidate(entry.symbols[j].string, entry.symbols[j].length);
            auto it = resolved.find(candidate);
            if (it != resolved.end())
            {
                address = it->second;
                break; // first match among this entry's candidates wins
            }
        }

        if (!address)
        {
            if (!entry.optional) allRequiredOk = false;
            continue;
        }

        if (!entry.hookFunction)
        {
            // hookFunction == nullptr means "just retrieve the address,
            // don't hook it".
            if (entry.pOriginalFunction) *entry.pOriginalFunction = address;
            continue;
        }

        if (!InternalWh_SetFunctionHook(mod, address, entry.hookFunction, entry.pOriginalFunction))
        {
            if (!entry.optional) allRequiredOk = false;
        }
    }

    return allRequiredOk;
}

// --- Not needed yet ---

HANDLE InternalWh_FindFirstSymbol4(void*, HMODULE, const WH_FIND_SYMBOL_OPTIONS*, WH_FIND_SYMBOL*)
{
    NOT_IMPLEMENTED();
    return nullptr;
}

BOOL InternalWh_FindNextSymbol2(void*, HANDLE, WH_FIND_SYMBOL*)
{
    NOT_IMPLEMENTED();
    return FALSE;
}

void InternalWh_FindCloseSymbol(void*, HANDLE)
{
    NOT_IMPLEMENTED();
}

BOOL InternalWh_Disasm(void*, void*, WH_DISASM_RESULT*)
{
    NOT_IMPLEMENTED();
    return FALSE;
}

// GetUrlContent: fetches a URL's content (e.g. for a mod that checks a
// remote config or update feed). Neither of our two mods calls this.
const WH_URL_CONTENT* InternalWh_GetUrlContent(void*, PCWSTR, const WH_GET_URL_CONTENT_OPTIONS*)
{
    NOT_IMPLEMENTED();
    return nullptr;
}

void InternalWh_FreeUrlContent(void*, const WH_URL_CONTENT* content)
{
    // Safe no-op: our GetUrlContent stub above never allocates anything,
    // so there's nothing to free. Revisit together if GetUrlContent ever
    // gets a real implementation.
    (void)content;
}
