#include "core/overlay_configuration.h"

#include "core/app_connection.h"
#include "core/icon_catalog.h"
#include "core/overlay_settings.h"

namespace
{
    SharedConfig::Layout g_fallbackLayout{};
    const SharedConfig::Layout* g_currentLayout = &g_fallbackLayout;
    OverlayConfiguration::ChangedCallback g_changedCallback = nullptr;

    void ApplyExternalConfiguration()
    {
        const auto* layout = AppConnection::GetLayout();
        if (!layout)
            return;

        g_currentLayout = layout;
        OverlaySettings::LoadExternal(*layout);
        IconCatalog::Reload(*layout);
    }

    void OnAppConfigurationChanged()
    {
        ApplyExternalConfiguration();
        if (g_changedCallback)
            g_changedCallback();
    }
}

namespace OverlayConfiguration
{
    bool Initialize(ChangedCallback changedCallback)
    {
        g_changedCallback = changedCallback;
        OverlaySettings::LoadWindhawkFallback();
        return EnsureConnected();
    }

    void Shutdown()
    {
        AppConnection::Disconnect();
        g_currentLayout = &g_fallbackLayout;
        g_changedCallback = nullptr;
    }

    bool EnsureConnected()
    {
        if (!AppConnection::IsConnected() && !AppConnection::Connect(&OnAppConfigurationChanged))
            return false;

        ApplyExternalConfiguration();
        return true;
    }

    bool IsEnabled()
    {
        return EnsureConnected() && AppConnection::IsEnabled();
    }

    const SharedConfig::Layout& Get()
    {
        return *g_currentLayout;
    }

    void ReloadWindhawkFallback()
    {
        OverlaySettings::LoadWindhawkFallback();
    }
} // namespace OverlayConfiguration
