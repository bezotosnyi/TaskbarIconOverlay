#include "logger.h"

#define LOGGER_EXPORT extern "C" __declspec(dllexport)

LOGGER_EXPORT void Logger_Init(HMODULE ownModule)
{
    if (ownModule == nullptr)
    {
        ownModule = GetModuleHandle(nullptr);
    }

    Logger::Init(ownModule);
}

LOGGER_EXPORT void Logger_Info(const wchar_t* text)
{
    if (text)
    {
        Logger::Info(std::wstring(text));
    }
}

LOGGER_EXPORT void Logger_Warn(const wchar_t* text)
{
    if (text)
    {
        Logger::Warn(std::wstring(text));
    }
}

LOGGER_EXPORT void Logger_Error(const wchar_t* text)
{
    if (text)
    {
        Logger::Error(std::wstring(text));
    }
}
