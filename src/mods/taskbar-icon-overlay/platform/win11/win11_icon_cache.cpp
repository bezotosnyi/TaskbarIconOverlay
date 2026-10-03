#include "platform/win11/win11_icon_cache.h"

#include "core/icon_catalog.h"

#include <winrt/Windows.Foundation.h>

#include <algorithm>
#include <mutex>
#include <string>
#include <unordered_map>

namespace
{
    using BitmapImage = winrt::Windows::UI::Xaml::Media::Imaging::BitmapImage;

    std::mutex g_mutex;
    std::unordered_map<std::wstring, BitmapImage> g_images;
}

namespace Win11IconCache
{
    BitmapImage Get(int position)
    {
        const std::wstring path = IconCatalog::GetPath(position);
        if (path.empty())
            return nullptr;

        std::lock_guard lock(g_mutex);
        if (const auto it = g_images.find(path); it != g_images.end())
            return it->second;

        try
        {
            std::wstring uriPath = L"file:///" + path;
            std::replace(uriPath.begin(), uriPath.end(), L'\\', L'/');

            BitmapImage bitmap;
            bitmap.UriSource(winrt::Windows::Foundation::Uri{ uriPath });
            g_images.emplace(path, bitmap);
            return bitmap;
        }
        catch (...)
        {
            return nullptr;
        }
    }
} // namespace Win11IconCache
