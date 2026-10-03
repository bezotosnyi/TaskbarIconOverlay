#pragma once

#include <windows.h>

class TaskbarBackend
{
public:
    virtual ~TaskbarBackend() = default;

    virtual BOOL Initialize() = 0;
    virtual void AfterInitialize() = 0;
    virtual void BeforeUninitialize() = 0;
    virtual void SettingsChanged() = 0;
};
