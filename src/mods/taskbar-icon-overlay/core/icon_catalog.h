#pragma once

#include "shared_config.h"

#include <string>

namespace IconCatalog
{
    // Replaces the catalog atomically from the current shared-memory snapshot.
    void Reload(const SharedConfig::Layout& config);

    // Returns the configured icon path for an assigned taskbar position.
    // An empty string means that no custom icon is configured for that position.
    std::wstring GetPath(int position);
} // namespace IconCatalog
