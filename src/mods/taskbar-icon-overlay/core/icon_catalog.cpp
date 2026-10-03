#include "core/icon_catalog.h"

#include <algorithm>
#include <mutex>
#include <string>
#include <unordered_map>

namespace
{
    std::mutex g_mutex;
    std::unordered_map<int, std::wstring> g_paths;
}

namespace IconCatalog
{
    void Reload(const SharedConfig::Layout& config)
    {
        std::lock_guard lock(g_mutex);
        g_paths.clear();

        const uint32_t count = std::min(config.windowCount, SharedConfig::kMaxIconSlots);
        for (uint32_t index = 0; index < count; ++index)
        {
            std::wstring path = config.iconPaths[index];
            if (!path.empty())
                g_paths.emplace(static_cast<int>(index + 1), std::move(path));
        }
    }

    std::wstring GetPath(int position)
    {
        std::lock_guard lock(g_mutex);
        const auto it = g_paths.find(position);
        return it == g_paths.end() ? std::wstring{} : it->second;
    }
} // namespace IconCatalog
