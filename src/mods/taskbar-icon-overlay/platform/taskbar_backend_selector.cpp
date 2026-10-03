#include "platform/taskbar_backend_selector.h"

#include "platform/win10/win10_backend.h"
#include "platform/win11/win11_backend.h"

#include <winternl.h>

namespace
{
    constexpr DWORD kWindows11FirstBuild = 22000;

    DWORD GetWindowsBuildNumber()
    {
        using RtlGetVersionFn = LONG (WINAPI*)(PRTL_OSVERSIONINFOW);

        const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        const auto rtlGetVersion = ntdll
            ? reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"))
            : nullptr;
        if (!rtlGetVersion)
            return 0;

        RTL_OSVERSIONINFOW versionInfo{};
        versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
        return rtlGetVersion(&versionInfo) == 0 ? versionInfo.dwBuildNumber : 0;
    }
}

TaskbarBackend& GetTaskbarBackend()
{
    static TaskbarBackend& backend = []() -> TaskbarBackend& {
        const DWORD build = GetWindowsBuildNumber();
        return build >= kWindows11FirstBuild ? GetWin11Backend() : GetWin10Backend();
    }();

    return backend;
}
