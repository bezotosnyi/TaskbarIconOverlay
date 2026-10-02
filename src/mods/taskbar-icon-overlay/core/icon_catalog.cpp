#include "icon_catalog.h"

#include <winrt/Windows.Foundation.h>

#include <algorithm>
#include <mutex>
#include <string>
#include <unordered_map>

namespace
{
    using BitmapImage = winrt::Windows::UI::Xaml::Media::Imaging::BitmapImage;

    class IconResource
    {
    public:
        explicit IconResource(std::wstring filePath) : m_filePath(std::move(filePath)) {}

        BitmapImage GetImage() const
        {
            if (m_cachedImage)
                return m_cachedImage;

            try
            {
                std::wstring uriPath = L"file:///" + m_filePath;
                std::replace(uriPath.begin(), uriPath.end(), L'\\', L'/');

                BitmapImage bitmap;
                bitmap.UriSource(winrt::Windows::Foundation::Uri{ uriPath });
                m_cachedImage = bitmap;
                return m_cachedImage;
            }
            catch (...)
            {
                return nullptr;
            }
        }

    private:
        std::wstring m_filePath;
        mutable BitmapImage m_cachedImage{ nullptr };
    };

    std::mutex g_mutex;
    std::unordered_map<int, IconResource> g_resources;
}

namespace IconCatalog
{
    void Reload(const SharedConfig::Layout& config)
    {
        std::lock_guard lock(g_mutex);
        g_resources.clear();

        const uint32_t count = std::min(config.windowCount, SharedConfig::kMaxIconSlots);
        for (uint32_t index = 0; index < count; ++index)
        {
            std::wstring path = config.iconPaths[index];
            if (!path.empty())
                g_resources.emplace(static_cast<int>(index + 1), IconResource(std::move(path)));
        }
    }

    BitmapImage Get(int position)
    {
        std::lock_guard lock(g_mutex);
        const auto it = g_resources.find(position);
        return it == g_resources.end() ? nullptr : it->second.GetImage();
    }
} // namespace IconCatalog
