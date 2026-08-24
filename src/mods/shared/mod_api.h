#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define MOD_API __declspec(dllexport)

extern "C"
{
    MOD_API void* WINAPI GetInternalModPtr();

    MOD_API BOOL WINAPI ModInit();
    MOD_API void WINAPI ModAfterInit();

    MOD_API void WINAPI ModBeforeUninit();
    MOD_API void WINAPI ModUninit();

    MOD_API BOOL WINAPI ModSettingsChanged(BOOL* bReload);
}
