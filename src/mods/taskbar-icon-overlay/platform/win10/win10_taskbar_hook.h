#pragma once

#include <windows.h>

namespace Win10TaskbarHook
{
    // Invoked after Explorer has completed its native button draw. The group
    // identity is opaque and must not be dereferenced outside the hook ABI.
    using ButtonDrawCallback = void (*)(void* taskButtonGroup, HDC hdc, const RECT& buttonRect);

    // Queues the Explorer symbol hook. Call ApplyPendingOperations after a
    // successful initialization to activate it.
    bool Initialize(ButtonDrawCallback buttonDrawCallback);
    void ApplyPendingOperations();
} // namespace Win10TaskbarHook
