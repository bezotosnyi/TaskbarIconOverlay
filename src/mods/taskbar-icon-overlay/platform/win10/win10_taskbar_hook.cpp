#include <windhawk_utils.h>

#include "platform/win10/win10_taskbar_hook.h"

namespace
{
    // BUTTONRENDERINFO starts with a DWORD followed by the rectangle for the
    // current button draw pass on Windows 10 19045. The remaining layout is
    // deliberately kept private until it is independently verified.
    struct ButtonRenderInfoPrefix
    {
        DWORD flags;
        RECT buttonRect;
    };

    using DrawRegularButtonFn = void (WINAPI*)(void* taskButtonGroup, HDC hdc,
        const ButtonRenderInfoPrefix* renderInfo);

    constexpr wchar_t kDrawRegularButtonSymbol[] =
        L"private: void __cdecl CTaskBtnGroup::_DrawRegularButton(struct HDC__ *,struct BUTTONRENDERINFO const &)";

    DrawRegularButtonFn g_drawRegularButtonOriginal = nullptr;
    Win10TaskbarHook::ButtonDrawCallback g_buttonDrawCallback = nullptr;
    LONG g_drawRegularButtonCount = 0;

    void WINAPI DrawRegularButtonHook(void* taskButtonGroup, HDC hdc,
        const ButtonRenderInfoPrefix* renderInfo)
    {
        g_drawRegularButtonOriginal(taskButtonGroup, hdc, renderInfo);

        if (!renderInfo)
            return;

        const RECT& rect = renderInfo->buttonRect;
        const LONG count = InterlockedIncrement(&g_drawRegularButtonCount);
        if (count == 1)
        {
            Wh_Log(L"Win10 _DrawRegularButton hook active: group=%p, rect=[%ld,%ld,%ld,%ld]",
                taskButtonGroup, rect.left, rect.top, rect.right, rect.bottom);
        }

        if (g_buttonDrawCallback)
            g_buttonDrawCallback(taskButtonGroup, hdc, rect);
    }
}

namespace Win10TaskbarHook
{
    bool Initialize(ButtonDrawCallback buttonDrawCallback)
    {
        g_buttonDrawCallback = buttonDrawCallback;

        HMODULE explorerModule = GetModuleHandleW(nullptr);
        if (!explorerModule)
        {
            Wh_Log(L"Win10 taskbar hook: explorer module handle unavailable");
            return false;
        }

        WindhawkUtils::SYMBOL_HOOK hooks[] = {
            {
                {kDrawRegularButtonSymbol},
                &g_drawRegularButtonOriginal,
                DrawRegularButtonHook,
            },
        };

        if (!HookSymbols(explorerModule, hooks, ARRAYSIZE(hooks)))
        {
            Wh_Log(L"Win10 taskbar hook: _DrawRegularButton symbol hook failed");
            return false;
        }

        Wh_Log(L"Win10 taskbar hook: _DrawRegularButton hook queued");
        return true;
    }

    void ApplyPendingOperations()
    {
        Wh_ApplyHookOperations();
    }
} // namespace Win10TaskbarHook
