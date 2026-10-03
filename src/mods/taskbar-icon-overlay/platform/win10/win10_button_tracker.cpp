#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windhawk_utils.h>

#include "platform/win10/win10_button_tracker.h"

#include <mutex>
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace
{
    struct ObservedButton
    {
        RECT rect{};
        int position = 0;
        int stickyPosition = 0;
    };

    std::mutex g_mutex;
    std::unordered_map<void*, ObservedButton> g_buttons;
    bool g_stickyBinding = false;

    bool AreEqual(const RECT& left, const RECT& right)
    {
        return left.left == right.left && left.top == right.top &&
            left.right == right.right && left.bottom == right.bottom;
    }

    void RecalculatePositions()
    {
        std::vector<std::pair<void*, ObservedButton*>> orderedButtons;
        orderedButtons.reserve(g_buttons.size());

        LONG minLeft = LONG_MAX;
        LONG maxLeft = LONG_MIN;
        LONG minTop = LONG_MAX;
        LONG maxTop = LONG_MIN;
        for (auto& [group, button] : g_buttons)
        {
            orderedButtons.emplace_back(group, &button);
            minLeft = std::min(minLeft, button.rect.left);
            maxLeft = std::max(maxLeft, button.rect.left);
            minTop = std::min(minTop, button.rect.top);
            maxTop = std::max(maxTop, button.rect.top);
        }

        const bool isVerticalTaskbar = (maxTop - minTop) > (maxLeft - minLeft);
        std::ranges::sort(orderedButtons, [isVerticalTaskbar](const auto& left, const auto& right) {
            const RECT& leftRect = left.second->rect;
            const RECT& rightRect = right.second->rect;
            const LONG leftPrimary = isVerticalTaskbar ? leftRect.top : leftRect.left;
            const LONG rightPrimary = isVerticalTaskbar ? rightRect.top : rightRect.left;
            if (leftPrimary != rightPrimary)
                return leftPrimary < rightPrimary;

            const LONG leftSecondary = isVerticalTaskbar ? leftRect.left : leftRect.top;
            const LONG rightSecondary = isVerticalTaskbar ? rightRect.left : rightRect.top;
            return leftSecondary < rightSecondary;
        });

        for (size_t index = 0; index < orderedButtons.size(); ++index)
        {
            auto& button = *orderedButtons[index].second;
            button.position = static_cast<int>(index + 1);
            if (!button.stickyPosition)
                button.stickyPosition = button.position;
        }
    }
}

namespace Win10ButtonTracker
{
    void Observe(void* taskButtonGroup, const RECT& buttonRect)
    {
        if (!taskButtonGroup)
            return;

        std::lock_guard lock(g_mutex);
        // When Explorer removes a button, the following button eventually
        // occupies the exact same final rect. Retaining both would leave a
        // phantom position in the tracker. A live button cannot keep that
        // rect, so replace its stale occupant.
        const auto collision = std::find_if(g_buttons.begin(), g_buttons.end(),
            [taskButtonGroup, &buttonRect](const auto& entry) {
                return entry.first != taskButtonGroup && AreEqual(entry.second.rect, buttonRect);
            });
        if (collision != g_buttons.end())
        {
            Wh_Log(L"Win10 button tracker: replacing stale group=%p at rect=[%ld,%ld,%ld,%ld]",
                collision->first, buttonRect.left, buttonRect.top, buttonRect.right, buttonRect.bottom);
            g_buttons.erase(collision);
        }

        auto [it, inserted] = g_buttons.try_emplace(taskButtonGroup, ObservedButton{ buttonRect });
        if (!inserted && AreEqual(it->second.rect, buttonRect))
            return;

        RECT oldRect{};
        if (!inserted)
        {
            oldRect = it->second.rect;
            it->second.rect = buttonRect;
        }

        RecalculatePositions();
        if (inserted)
        {
            Wh_Log(L"Win10 button tracker: discovered group=%p, position=%d, rect=[%ld,%ld,%ld,%ld], total=%zu",
                taskButtonGroup,
                it->second.position,
                buttonRect.left, buttonRect.top, buttonRect.right, buttonRect.bottom,
                g_buttons.size());
            return;
        }

        Wh_Log(L"Win10 button tracker: group=%p, position=%d, moved [%ld,%ld,%ld,%ld] -> [%ld,%ld,%ld,%ld]",
            taskButtonGroup,
            it->second.position,
            oldRect.left, oldRect.top, oldRect.right, oldRect.bottom,
            buttonRect.left, buttonRect.top, buttonRect.right, buttonRect.bottom);
    }

    void SetStickyBinding(bool enabled)
    {
        std::lock_guard lock(g_mutex);
        if (enabled && !g_stickyBinding)
        {
            for (auto& [_, button] : g_buttons)
                button.stickyPosition = button.position;
        }
        g_stickyBinding = enabled;
    }

    int GetOverlayPosition(void* taskButtonGroup, bool stickyBinding)
    {
        std::lock_guard lock(g_mutex);
        const auto it = g_buttons.find(taskButtonGroup);
        if (it == g_buttons.end())
            return 0;

        return stickyBinding ? it->second.stickyPosition : it->second.position;
    }
} // namespace Win10ButtonTracker
