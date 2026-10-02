#pragma once

#include "shared_config.h"

#include <winrt/Windows.UI.Xaml.Media.Imaging.h>

namespace IconCatalog
{
    // Replaces the catalog atomically from the current shared-memory snapshot.
    void Reload(const SharedConfig::Layout& config);

    // Returns a cached BitmapImage for an assigned taskbar position, or null.
    winrt::Windows::UI::Xaml::Media::Imaging::BitmapImage Get(int position);
} // namespace IconCatalog
