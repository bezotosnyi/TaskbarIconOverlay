// ==WindhawkMod==
// @id              taskbar-icon-overlay
// @name            Taskbar Icon Overlay
// @description     Displays custom icon and number overlays on taskbar buttons.
// @version         0.2.0
// @author          Dmytro Bezotosnyi
// @github          https://github.com/bezotosnyi/TaskbarIconOverlay
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lole32 -loleaut32 -lruntimeobject
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Taskbar Icon Overlay

Adds configurable icon and number overlays to taskbar buttons.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- numberPosition: topLeft
  $name: Number position
  $options:
  - topLeft: Top left
  - topRight: Top right
  - bottomLeft: Bottom left
  - bottomRight: Bottom right
- numberSize: 12
  $name: Number font size
  $description: Size of the overlay numbers (8-16)
- numberColor: "#FFFFFF"
  $name: Number color
  $description: Text color (#RRGGBB or #AARRGGBB)
- backgroundColor: "#80000000"
  $name: Stroke/outline color
  $description: Outline color (#RRGGBB or #AARRGGBB)
- showOnAllTaskbars: false
  $name: Show overlays on all taskbars
*/
// ==/WindhawkModSettings==

#include "platform/win11/win11_backend.h"

// This file is deliberately limited to Windhawk metadata and lifecycle
// dispatch. Platform-specific taskbar code belongs to a backend.
BOOL Wh_ModInit()
{
    return GetWin11Backend().Initialize();
}

void Wh_ModAfterInit()
{
    GetWin11Backend().AfterInitialize();
}

void Wh_ModBeforeUninit()
{
    GetWin11Backend().BeforeUninitialize();
}

void Wh_ModUninit()
{
}

void Wh_ModSettingsChanged()
{
    GetWin11Backend().SettingsChanged();
}
