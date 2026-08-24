#include "mod_api.h"

#include "windhawk_api_internal.h"

// Forward Declarations from mod's main source file (taskbar-icon-overlay.wh.cpp)
BOOL Wh_ModInit();
void Wh_ModAfterInit();
void Wh_ModBeforeUninit();
void Wh_ModUninit();
void Wh_ModSettingsChanged();

extern "C"
{
    void* WINAPI GetInternalModPtr()
    {
        return InternalWhModPtr;
    }

    BOOL WINAPI ModInit()
    {
        return Wh_ModInit();
    }

    void WINAPI ModAfterInit()
    {
        Wh_ModAfterInit();
    }

    void WINAPI ModBeforeUninit()
    {
        Wh_ModBeforeUninit();
    }

    void WINAPI ModUninit()
    {
        Wh_ModUninit();
    }

    BOOL WINAPI ModSettingsChanged(BOOL* bReload)
    {
        (void)(bReload);
        Wh_ModSettingsChanged();
        return TRUE;
    }
} // extern "C"
