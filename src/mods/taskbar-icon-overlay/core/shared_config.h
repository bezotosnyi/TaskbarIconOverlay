#pragma once

#include <cstdint>

// ABI contract with TaskbarIconOverlay.App/Services/SharedConfigWriter.cs.
// Keep field order, packing and fixed-size buffers in sync with the C# writer.
namespace SharedConfig
{
    constexpr uint32_t kMaxIconSlots = 50;
    constexpr uint32_t kMaxPathChars = 260;

    constexpr wchar_t kMemName[] = L"Local\\TaskbarIconOverlay_Config";
    constexpr wchar_t kEnabledEventName[] = L"Local\\TaskbarIconOverlay_Enabled";
    constexpr wchar_t kConfigChangedEventName[] = L"Local\\TaskbarIconOverlay_ConfigChanged";

    enum class NumberPosition : uint32_t
    {
        TopLeft = 0,
        TopRight = 1,
        BottomLeft = 2,
        BottomRight = 3,
    };

#pragma pack(push, 1)
    struct Layout
    {
        uint32_t version;
        uint32_t windowCount;
        wchar_t iconPaths[kMaxIconSlots][kMaxPathChars];
        uint32_t stickyIconBinding;
        uint32_t numberedCount;
        uint32_t allowNumbersBeyondTen;
        NumberPosition numberPosition;
        uint32_t numberSize;
        uint32_t numberColorArgb;
        uint32_t backgroundColorArgb;
        uint32_t showOnAllTaskbars;
    };
#pragma pack(pop)
} // namespace SharedConfig
