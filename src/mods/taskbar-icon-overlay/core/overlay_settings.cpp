#include "overlay_settings.h"

#include <windhawk_utils.h>

#include <algorithm>
#include <format>

namespace
{
    OverlaySettings::Values g_values;

    int ClampNumberSize(int value)
    {
        return std::clamp(value, 8, 16);
    }
}

namespace OverlaySettings
{
    const Values& Get()
    {
        return g_values;
    }

    void LoadWindhawkFallback()
    {
        PCWSTR position = Wh_GetStringSetting(L"numberPosition");
        g_values.numberPosition = NumberPosition::BottomRight;
        if (wcscmp(position, L"topLeft") == 0) g_values.numberPosition = NumberPosition::TopLeft;
        else if (wcscmp(position, L"topRight") == 0) g_values.numberPosition = NumberPosition::TopRight;
        else if (wcscmp(position, L"bottomLeft") == 0) g_values.numberPosition = NumberPosition::BottomLeft;
        else if (wcscmp(position, L"bottomRight") == 0) g_values.numberPosition = NumberPosition::BottomRight;
        Wh_FreeStringSetting(position);

        g_values.numberSize = ClampNumberSize(Wh_GetIntSetting(L"numberSize"));

        PCWSTR numberColor = Wh_GetStringSetting(L"numberColor");
        g_values.numberColor = numberColor;
        Wh_FreeStringSetting(numberColor);

        PCWSTR backgroundColor = Wh_GetStringSetting(L"backgroundColor");
        g_values.backgroundColor = backgroundColor;
        Wh_FreeStringSetting(backgroundColor);

        g_values.showOnAllTaskbars = Wh_GetIntSetting(L"showOnAllTaskbars") != 0;
    }

    void LoadExternal(const SharedConfig::Layout& config)
    {
        const auto rawPosition = static_cast<uint32_t>(config.numberPosition);
        g_values.numberPosition = rawPosition <= static_cast<uint32_t>(SharedConfig::NumberPosition::BottomRight)
            ? static_cast<NumberPosition>(rawPosition)
            : NumberPosition::BottomRight;
        g_values.numberSize = ClampNumberSize(static_cast<int>(config.numberSize));
        g_values.numberColor = std::format(L"#{:08X}", config.numberColorArgb);
        g_values.backgroundColor = std::format(L"#{:08X}", config.backgroundColorArgb);
        g_values.showOnAllTaskbars = config.showOnAllTaskbars != 0;
    }
} // namespace OverlaySettings
