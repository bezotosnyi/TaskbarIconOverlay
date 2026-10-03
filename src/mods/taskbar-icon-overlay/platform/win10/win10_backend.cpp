#include <windhawk_utils.h>

#include "core/overlay_configuration.h"
#include "platform/win10/win10_backend.h"
#include "platform/win10/win10_button_tracker.h"
#include "platform/win10/win10_overlay_renderer.h"
#include "platform/win10/win10_taskbar_hook.h"

namespace
{
    void OnTaskbarButtonDraw(void* taskButtonGroup, HDC hdc, const RECT& buttonRect)
    {
        Win10ButtonTracker::Observe(taskButtonGroup, buttonRect);

        if (!OverlayConfiguration::IsEnabled())
            return;

        const auto& config = OverlayConfiguration::Get();
        const int currentPosition = Win10ButtonTracker::GetOverlayPosition(taskButtonGroup, false);
        const int iconPosition = Win10ButtonTracker::GetOverlayPosition(
            taskButtonGroup, config.stickyIconBinding);
        Win10OverlayRenderer::Draw(hdc, buttonRect, iconPosition, currentPosition);
    }

    void OnConfigurationChanged()
    {
        Win10ButtonTracker::SetStickyBinding(OverlayConfiguration::Get().stickyIconBinding);
        Win10OverlayRenderer::ClearCache();
        Win10OverlayRenderer::RefreshTaskbars();
    }

    class Win10Backend final : public TaskbarBackend
    {
    public:
        BOOL Initialize() override
        {
            if (!Win10OverlayRenderer::Initialize())
            {
                Wh_Log(L"Win10 backend: native overlay renderer initialization failed");
                return FALSE;
            }

            if (!OverlayConfiguration::Initialize(&OnConfigurationChanged))
            {
                Wh_Log(L"Win10 backend: App configuration is unavailable; using Windhawk fallback settings");
            }

            if (!Win10TaskbarHook::Initialize(&OnTaskbarButtonDraw))
            {
                Wh_Log(L"Win10 backend: _DrawRegularButton symbol hook failed");
                OverlayConfiguration::Shutdown();
                Win10OverlayRenderer::Shutdown();
                return FALSE;
            }

            OnConfigurationChanged();

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
            Win10OverlayRenderer::Shutdown();
        }

        void SettingsChanged() override
        {
            OverlayConfiguration::ReloadWindhawkFallback();
            OnConfigurationChanged();
        }
    };
}

TaskbarBackend& GetWin10Backend()
{
    static Win10Backend backend;
    return backend;
}
