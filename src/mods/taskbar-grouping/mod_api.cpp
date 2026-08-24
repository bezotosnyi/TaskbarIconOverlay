#include "mod_api.h"

#include "windhawk_api_internal.h"

// Forward Declarations from mod's main source file (taskbar-grouping.wh.cpp)
BOOL Wh_ModInit();
void Wh_ModAfterInit();
void Wh_ModUninit();
BOOL Wh_ModSettingsChanged(BOOL* bReload);

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
        // Not implemented by mod
    }

    void WINAPI ModUninit()
    {
        Wh_ModUninit();
    }

    BOOL WINAPI ModSettingsChanged(BOOL* bReload)
    {
        return Wh_ModSettingsChanged(bReload);
    }
} // extern "C"
