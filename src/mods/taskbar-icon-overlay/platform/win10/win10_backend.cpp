#include <windhawk_utils.h>

#include "core/overlay_configuration.h"
#include "platform/win10/win10_backend.h"

namespace
{
    // BUTTONRENDERINFO starts with a DWORD followed by the rectangle for the
    // current button draw pass on Windows 10 19045. The structure remains
    // opaque until all fields needed by the native renderer are documented.
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
    LONG g_drawRegularButtonCount = 0;

    void WINAPI DrawRegularButtonHook(void* taskButtonGroup, HDC hdc,
        const ButtonRenderInfoPrefix* renderInfo)
    {
        g_drawRegularButtonOriginal(taskButtonGroup, hdc, renderInfo);

        // Intentionally passive for the first integration step. The probe
        // already established that this hook is the correct per-button draw
        // location; production rendering will be added after native resource
        // ownership and button numbering are implemented.
        const LONG count = InterlockedIncrement(&g_drawRegularButtonCount);
        if (count == 1 && renderInfo)
        {
            const RECT& rect = renderInfo->buttonRect;
            Wh_Log(L"Win10 _DrawRegularButton hook active: group=%p, rect=[%ld,%ld,%ld,%ld]",
                taskButtonGroup, rect.left, rect.top, rect.right, rect.bottom);
        }
    }

    class Win10Backend final : public TaskbarBackend
    {
    public:
        BOOL Initialize() override
        {
            if (!OverlayConfiguration::Initialize(nullptr))
            {
                Wh_Log(L"Win10 backend: App configuration is unavailable; using Windhawk fallback settings");
            }

            HMODULE explorerModule = GetModuleHandleW(nullptr);
            if (!explorerModule)
            {
                Wh_Log(L"Win10 backend: explorer module handle unavailable");
                return FALSE;
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
                Wh_Log(L"Win10 backend: _DrawRegularButton symbol hook failed");
                return FALSE;
            }

            Wh_Log(L"Win10 backend: _DrawRegularButton hook queued");
            return TRUE;
        }

        void AfterInitialize() override
        {
            Wh_ApplyHookOperations();
        }

        void BeforeUninitialize() override
        {
            OverlayConfiguration::Shutdown();
        }

        void SettingsChanged() override
        {
            OverlayConfiguration::ReloadWindhawkFallback();
        }
    };
}

TaskbarBackend& GetWin10Backend()
{
    static Win10Backend backend;
    return backend;
}
