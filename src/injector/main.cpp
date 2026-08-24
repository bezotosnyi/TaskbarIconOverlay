// main.cpp — TaskbarIconOverlay.Injector
//
// CLI: enable | disable | status
//
// enable:  LoadLibraryW injection of TaskbarIconOverlay.Engine.dll into
//          explorer.exe. DllMain spins up the worker thread itself
//          (symbol resolution, hooks) - injector doesn't call anything
//          separately after LoadLibrary succeeds.
// disable: RVA-based remote call to the exported EngineShutdown - removes
//          hooks and cleanly unloads the DLL, WITHOUT restarting
//          explorer.exe.
// status:  whether TaskbarIconOverlay.Engine.dll is currently loaded in
//          explorer.exe.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <tlhelp32.h>
#include <psapi.h>
#include <shlwapi.h>

#include <iostream>
#include <optional>
#include <string>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shlwapi.lib")

namespace
{
    constexpr wchar_t kEngineDllName[] = L"TaskbarIconOverlay.Engine.dll";
    constexpr char kShutdownExportName[] = "EngineShutdown";

    // All exit codes live in one place, including status results (not just
    // failures) - the App layer parses these directly rather than stderr
    // text, since localization happens at the App level, not here.
    enum class ExitCode : int
    {
        Success = 0,
        UsageError = 2, // missing/unknown command

        ExplorerNotFound = 10,
        OpenProcessFailed = 11,
        WindhawkConflict = 12, // real Windhawk already in explorer.exe
        EngineDllNotFound = 13, // Engine.dll missing next to injector.exe
        InjectionFailed = 14, // remote LoadLibraryW returned NULL
        RemoteExportCallFailed = 15, // CreateRemoteThread on EngineShutdown failed

        // status-specific results (not errors)
        Enabled = 0, // same value as Success - "status" succeeded AND engine is enabled
        Disabled = 1,
    };

    int AsInt(ExitCode code)
    {
        return static_cast<int>(code);
    }

    std::wstring GetExeDir()
    {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        PathRemoveFileSpecW(path);
        return path;
    }

    std::optional<DWORD> FindExplorerPid()
    {
        const auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return std::nullopt;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        std::optional<DWORD> result;
        if (Process32FirstW(snap, &entry))
        {
            do
            {
                if (_wcsicmp(entry.szExeFile, L"explorer.exe") == 0)
                {
                    result = entry.th32ProcessID;
                    break;
                }
            }
            while (Process32NextW(snap, &entry));
        }

        CloseHandle(snap);
        return result;
    }

    // Finds the base address of an already-loaded module in a FOREIGN
    // process by file name. Used both for the real-Windhawk conflict check
    // and for our own status/RVA calculations.
    std::optional<HMODULE> FindRemoteModuleBase(HANDLE proc, const std::wstring& moduleFileName)
    {
        HMODULE modules[1024];
        DWORD needed;
        if (!EnumProcessModulesEx(proc, modules, sizeof(modules), &needed, LIST_MODULES_ALL))
        {
            return std::nullopt;
        }

        const DWORD count = needed / sizeof(HMODULE);
        for (DWORD i = 0; i < count; i++)
        {
            wchar_t name[MAX_PATH];
            if (GetModuleBaseNameW(proc, modules[i], name, MAX_PATH) &&
                _wcsicmp(name, moduleFileName.c_str()) == 0)
            {
                return modules[i];
            }
        }

        return std::nullopt;
    }

    // LoadLibraryW injection. Returns the process HANDLE (left open for the
    // caller to use further) or nullptr on failure.
    HANDLE InjectDll(DWORD pid, const std::wstring& dllPath)
    {
        const HANDLE proc = OpenProcess(
            PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
            PROCESS_VM_WRITE | PROCESS_VM_READ,
            FALSE, pid);
        if (!proc)
        {
            std::wcerr << L"OpenProcess failed: " << GetLastError() << L"\n";
            return nullptr;
        }

        const auto bufSize = (dllPath.size() + 1) * sizeof(wchar_t);
        const auto remoteMem = VirtualAllocEx(proc, nullptr, bufSize, MEM_COMMIT, PAGE_READWRITE);
        if (!remoteMem)
        {
            std::wcerr << L"VirtualAllocEx failed: " << GetLastError() << L"\n";
            CloseHandle(proc);
            return nullptr;
        }

        if (!WriteProcessMemory(proc, remoteMem, dllPath.c_str(), bufSize, nullptr))
        {
            std::wcerr << L"WriteProcessMemory failed: " << GetLastError() << L"\n";
            VirtualFreeEx(proc, remoteMem, 0, MEM_RELEASE);
            CloseHandle(proc);
            return nullptr;
        }

        const auto loadLibraryAddr = reinterpret_cast<LPTHREAD_START_ROUTINE>(
            GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));

        const auto thread = CreateRemoteThread(proc, nullptr, 0, loadLibraryAddr, remoteMem, 0, nullptr);
        if (!thread)
        {
            std::wcerr << L"CreateRemoteThread (LoadLibraryW) failed: " << GetLastError() << L"\n";
            VirtualFreeEx(proc, remoteMem, 0, MEM_RELEASE);
            CloseHandle(proc);
            return nullptr;
        }

        WaitForSingleObject(thread, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeThread(thread, &exitCode);

        CloseHandle(thread);
        VirtualFreeEx(proc, remoteMem, 0, MEM_RELEASE);

        if (exitCode == 0)
        {
            std::wcerr << L"LoadLibraryW returned NULL in the remote process - DLL failed to load\n";
            CloseHandle(proc);
            return nullptr;
        }

        return proc;
    }

    // Calls a remote export by name: computes the RVA locally (via a
    // temporary LoadLibrary of the same DLL in our own process) and adds it
    // to the real base address in the foreign process. argString is an
    // optional wide string passed as the thread parameter (empty => nullptr
    // argument).
    bool CallRemoteExport(HANDLE proc, HMODULE remoteBase, const std::wstring& dllPath,
                          const char* exportName, const std::wstring& argString)
    {
        const auto localModule = LoadLibraryW(dllPath.c_str());
        if (!localModule)
        {
            std::wcerr << L"Local LoadLibrary (for RVA calculation) failed\n";
            return false;
        }

        const auto localFunc = GetProcAddress(localModule, exportName);
        if (!localFunc)
        {
            std::wcerr << L"Export " << exportName << L" not found\n";
            FreeLibrary(localModule);
            return false;
        }

        const ptrdiff_t rva = reinterpret_cast<uint8_t*>(localFunc) -
            reinterpret_cast<uint8_t*>(localModule);
        const auto remoteFunc = reinterpret_cast<LPTHREAD_START_ROUTINE>(
            reinterpret_cast<uint8_t*>(remoteBase) + rva);

        FreeLibrary(localModule); // local copy no longer needed

        LPVOID remoteArgMem = nullptr;
        if (!argString.empty())
        {
            const auto bufSize = (argString.size() + 1) * sizeof(wchar_t);
            remoteArgMem = VirtualAllocEx(proc, nullptr, bufSize, MEM_COMMIT, PAGE_READWRITE);
            if (!remoteArgMem ||
                !WriteProcessMemory(proc, remoteArgMem, argString.c_str(), bufSize, nullptr))
            {
                std::wcerr << L"Failed to write the argument into the foreign process\n";
                return false;
            }
        }

        const auto thread = CreateRemoteThread(proc, nullptr, 0, remoteFunc, remoteArgMem, 0, nullptr);
        if (!thread)
        {
            std::wcerr << L"CreateRemoteThread (" << exportName << L") failed: "
                << GetLastError() << L"\n";
            if (remoteArgMem) VirtualFreeEx(proc, remoteArgMem, 0, MEM_RELEASE);
            return false;
        }

        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
        if (remoteArgMem)
        {
            VirtualFreeEx(proc, remoteArgMem, 0, MEM_RELEASE);
        }

        return true;
    }

    int CmdEnable()
    {
        const auto pid = FindExplorerPid();
        if (!pid)
        {
            std::wcerr << L"explorer.exe not found\n";
            return AsInt(ExitCode::ExplorerNotFound);
        }

        std::wcout << L"explorer.exe PID = " << *pid << L"\n";

        const auto checkProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, *pid);
        if (!checkProc)
        {
            std::wcerr << L"OpenProcess (check) failed: " << GetLastError() << L"\n";
            return AsInt(ExitCode::OpenProcessFailed);
        }

        // Conflict with a real Windhawk install: two independent MinHook
        // instances patching the same explorer.exe memory - undefined
        // behavior.
        if (FindRemoteModuleBase(checkProc, L"windhawk.dll").has_value())
        {
            std::wcerr << L"windhawk.dll detected inside explorer.exe. "
                L"Disable Windhawk before enabling TaskbarIconOverlay.\n";
            CloseHandle(checkProc);
            return AsInt(ExitCode::WindhawkConflict);
        }

        if (FindRemoteModuleBase(checkProc, kEngineDllName).has_value())
        {
            std::wcout << L"Already enabled (Engine.dll already in explorer.exe).\n";
            CloseHandle(checkProc);
            return AsInt(ExitCode::Success);
        }

        CloseHandle(checkProc);

        const auto enginePath = GetExeDir() + L"\\" + kEngineDllName;
        if (!PathFileExistsW(enginePath.c_str()))
        {
            std::wcerr << L"Not found: " << enginePath << L"\n";
            return AsInt(ExitCode::EngineDllNotFound);
        }

        const auto proc = InjectDll(*pid, enginePath);
        if (!proc)
        {
            return AsInt(ExitCode::InjectionFailed);
        }

        CloseHandle(proc);

        std::wcout << L"Enabled.\n";
        return AsInt(ExitCode::Success);
    }

    int CmdDisable()
    {
        const auto pid = FindExplorerPid();
        if (!pid)
        {
            std::wcerr << L"explorer.exe not found\n";
            return AsInt(ExitCode::ExplorerNotFound);
        }

        const auto proc = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ |
            PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
            FALSE, *pid);
        if (!proc)
        {
            std::wcerr << L"OpenProcess failed: " << GetLastError() << L"\n";
            return AsInt(ExitCode::OpenProcessFailed);
        }

        const auto remoteBase = FindRemoteModuleBase(proc, kEngineDllName);
        if (!remoteBase)
        {
            std::wcout << L"Already disabled (Engine.dll not loaded).\n";
            CloseHandle(proc);
            return AsInt(ExitCode::Success);
        }

        const auto enginePath = GetExeDir() + L"\\" + kEngineDllName;
        const auto ok = CallRemoteExport(proc, *remoteBase, enginePath, kShutdownExportName, L"");
        CloseHandle(proc);

        if (!ok)
        {
            std::wcerr << L"Call to " << kShutdownExportName << L" failed.\n";
            return AsInt(ExitCode::RemoteExportCallFailed);
        }

        std::wcout << L"Disabled (clean unhook, no explorer.exe restart).\n";
        return AsInt(ExitCode::Success);
    }

    int CmdStatus()
    {
        const auto pid = FindExplorerPid();
        if (!pid)
        {
            std::wcerr << L"explorer.exe not found\n";
            return AsInt(ExitCode::ExplorerNotFound);
        }

        const auto proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, *pid);
        if (!proc)
        {
            std::wcerr << L"OpenProcess failed: " << GetLastError() << L"\n";
            return AsInt(ExitCode::OpenProcessFailed);
        }

        const auto enabled = FindRemoteModuleBase(proc, kEngineDllName).has_value();
        CloseHandle(proc);

        std::wcout << (enabled ? L"enabled\n" : L"disabled\n");
        return AsInt(enabled ? ExitCode::Enabled : ExitCode::Disabled);
    }
} // namespace

int wmain(int argc, wchar_t* argv[])
{
    if (argc < 2)
    {
        std::wcerr << L"Usage: injector.exe enable|disable|status\n";
        return AsInt(ExitCode::UsageError);
    }

    const std::wstring cmd = argv[1];
    if (cmd == L"enable") return CmdEnable();
    if (cmd == L"disable") return CmdDisable();
    if (cmd == L"status") return CmdStatus();

    std::wcerr << L"Unknown command: " << cmd << L"\n";
    return AsInt(ExitCode::UsageError);
}
