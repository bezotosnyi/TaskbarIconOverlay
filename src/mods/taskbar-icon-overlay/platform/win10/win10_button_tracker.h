#pragma once

#include <windows.h>

namespace Win10ButtonTracker
{
    // Records a native task button group observed at the verified Explorer
    // draw point. The group pointer is treated as an opaque identity only.
    void Observe(void* taskButtonGroup, const RECT& buttonRect);

    // Captures the current positions as stable assignments when sticky binding
    // becomes enabled. Turning it off returns to live positional assignment.
    void SetStickyBinding(bool enabled);

    // Returns the one-based position for an observed group. Sticky binding
    // uses the captured group assignment; otherwise it follows live order.
    int GetOverlayPosition(void* taskButtonGroup, bool stickyBinding);
} // namespace Win10ButtonTracker
