#pragma once

#include <windows.h>

namespace Win10OverlayRenderer
{
    bool Initialize();
    void Shutdown();
    void ClearCache();
    void RefreshTaskbars();
    void Draw(HDC hdc, const RECT& buttonRect, int iconPosition, int numberPosition);
} // namespace Win10OverlayRenderer
