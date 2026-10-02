#include "app_connection.h"

#include <windows.h>

namespace
{
    HANDLE g_mapping = nullptr;
    const SharedConfig::Layout* g_layout = nullptr;
    HANDLE g_enabledEvent = nullptr;
    HANDLE g_configChangedEvent = nullptr;
    HANDLE g_watcherThread = nullptr;
    volatile bool g_watcherShouldStop = false;
    AppConnection::ConfigChangedCallback g_configChangedCallback = nullptr;

    DWORD WINAPI ConfigWatcherThreadProc(void*)
    {
        if (!g_configChangedEvent)
            return 0;

        while (!g_watcherShouldStop)
        {
            const DWORD result = WaitForSingleObject(g_configChangedEvent, 500);
            if (g_watcherShouldStop)
                break;
            if (result == WAIT_OBJECT_0 && g_configChangedCallback)
                g_configChangedCallback();
        }
        return 0;
    }
}

namespace AppConnection
{
    bool Connect(ConfigChangedCallback configChangedCallback)
    {
        if (g_mapping)
            return true;

        g_mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, SharedConfig::kMemName);
        if (!g_mapping)
            return false;

        g_layout = static_cast<const SharedConfig::Layout*>(
            MapViewOfFile(g_mapping, FILE_MAP_READ, 0, 0, sizeof(SharedConfig::Layout)));
        if (!g_layout)
        {
            CloseHandle(g_mapping);
            g_mapping = nullptr;
            return false;
        }

        g_enabledEvent = OpenEventW(SYNCHRONIZE, FALSE, SharedConfig::kEnabledEventName);
        g_configChangedEvent = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE,
            SharedConfig::kConfigChangedEventName);
        g_configChangedCallback = configChangedCallback;
        g_watcherShouldStop = false;
        g_watcherThread = CreateThread(nullptr, 0, ConfigWatcherThreadProc, nullptr, 0, nullptr);
        return true;
    }

    void Disconnect()
    {
        g_watcherShouldStop = true;
        if (g_configChangedEvent)
            SetEvent(g_configChangedEvent);
        if (g_watcherThread)
        {
            WaitForSingleObject(g_watcherThread, 2000);
            CloseHandle(g_watcherThread);
            g_watcherThread = nullptr;
        }

        if (g_layout)
            UnmapViewOfFile(g_layout);
        if (g_mapping)
            CloseHandle(g_mapping);
        if (g_enabledEvent)
            CloseHandle(g_enabledEvent);
        if (g_configChangedEvent)
            CloseHandle(g_configChangedEvent);

        g_mapping = g_enabledEvent = g_configChangedEvent = nullptr;
        g_layout = nullptr;
        g_configChangedCallback = nullptr;
    }

    bool IsConnected()
    {
        return g_layout != nullptr;
    }

    bool IsEnabled()
    {
        return g_enabledEvent && WaitForSingleObject(g_enabledEvent, 0) == WAIT_OBJECT_0;
    }

    const SharedConfig::Layout* GetLayout()
    {
        return g_layout;
    }
} // namespace AppConnection
