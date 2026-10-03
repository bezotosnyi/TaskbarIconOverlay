#pragma once

#include "core/shared_config.h"

namespace OverlayConfiguration
{
    using ChangedCallback = void (*)();

    // Starts with Windhawk fallback settings, then adopts the App's shared
    // configuration when it is available. The callback runs after a config
    // change has been applied to the core services.
    bool Initialize(ChangedCallback changedCallback);
    void Shutdown();

    // Attempts a late connection when the App starts after Explorer/mod load.
    bool EnsureConnected();
    bool IsEnabled();
    const SharedConfig::Layout& Get();

    void ReloadWindhawkFallback();
} // namespace OverlayConfiguration
