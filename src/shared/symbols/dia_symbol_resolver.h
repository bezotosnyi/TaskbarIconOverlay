#pragma once
// Resolves symbols in a target module by undecorated name, through a
// private MS DIA + SymSrv pair placed next to the caller's own module.
//
// This is a static library: every DLL that links it (WindhawkWrapper
// inside each mod DLL, Engine.dll for the numbering feature) gets its own
// independent copy of any instance's state - there's no cross-DLL
// sharing. Keep one long-lived instance per DLL (e.g. a function-local
// static, lazily constructed on first use) so its per-module cache
// actually pays off across repeated calls within that DLL's lifetime.
//
// Failures and results are logged directly via Logger as they happen -
// call Logger::Init() before the first FindSymbol/FindSymbols call in
// your DLL, or those lines are silently dropped (Logger::* no-op until
// initialized).

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>
#include <unordered_map>
#include <vector>

class DiaSymbolResolver
{
public:
    // verboseTrace: when true, logs step-by-step detail (DIA session
    // setup, per-symbol matches) at Info level in addition to the normal
    // Warn-level failures. Keep false for normal operation - the extra
    // detail is only useful while debugging the resolver itself.
    explicit DiaSymbolResolver(bool verboseTrace = false);

    // Single-symbol lookup - convenience wrapper over FindSymbols below.
    void* FindSymbol(HMODULE engineModule, HMODULE targetModule,
                     const wchar_t* undecoratedName);

    // Resolves an entire list of signatures against targetModule. Names
    // already present in this instance's cache for that module are
    // returned immediately; only the remaining names trigger a DIA
    // session, and only if at least one name isn't cached yet - if
    // everything requested is already cached, no DIA session happens at
    // all. Returns a map containing only the signatures that were
    // actually found - a missing entry means "not found"; it's up to the
    // caller to decide whether that's fatal (also logged as a warning
    // either way).
    std::unordered_map<std::wstring, void*> FindSymbols(
        HMODULE engineModule, HMODULE targetModule,
        const std::vector<std::wstring>& undecoratedNames);

private:
    bool m_verbose;
    std::unordered_map<HMODULE, std::unordered_map<std::wstring, void*>> m_cache;
};
