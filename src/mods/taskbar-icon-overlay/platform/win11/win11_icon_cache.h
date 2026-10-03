#pragma once

#include <winrt/Windows.UI.Xaml.Media.Imaging.h>

namespace Win11IconCache
{
    // Decodes and caches the currently configured image for a taskbar position.
    winrt::Windows::UI::Xaml::Media::Imaging::BitmapImage Get(int position);
} // namespace Win11IconCache
