#pragma once

#include "core/shared_config.h"

#include <string>

namespace OverlaySettings
{
    enum class NumberPosition
    {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    struct Values
    {
        NumberPosition numberPosition = NumberPosition::BottomRight;
        int numberSize = 12;
        std::wstring numberColor = L"#FFFFFF";
        std::wstring backgroundColor = L"#80000000";
        bool showOnAllTaskbars = false;
    };

    const Values& Get();
    void LoadWindhawkFallback();
    void LoadExternal(const SharedConfig::Layout& config);
} // namespace OverlaySettings
