#include <windhawk_utils.h>

#include "core/overlay_configuration.h"
#include "platform/win10/win10_backend.h"
#include "platform/win10/win10_taskbar_hook.h"

namespace
{
    class Win10Backend final : public TaskbarBackend
    {
    public:
        BOOL Initialize() override
        {
            if (!OverlayConfiguration::Initialize(nullptr))
            {
                Wh_Log(L"Win10 backend: App configuration is unavailable; using Windhawk fallback settings");
            }

            if (!Win10TaskbarHook::Initialize(nullptr))
            {
                Wh_Log(L"Win10 backend: _DrawRegularButton symbol hook failed");
                return FALSE;
            }

            Wh_Log(L"Win10 backend: _DrawRegularButton hook queued");
            return TRUE;
        }

        void AfterInitialize() override
        {
            Win10TaskbarHook::ApplyPendingOperations();
        }

        void BeforeUninitialize() override
        {
            OverlayConfiguration::Shutdown();
        }

        void SettingsChanged() override
        {
            OverlayConfiguration::ReloadWindhawkFallback();
        }
    };
}

TaskbarBackend& GetWin10Backend()
{
    static Win10Backend backend;
    return backend;
}
