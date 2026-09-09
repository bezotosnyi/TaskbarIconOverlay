using System;
using TaskbarIconOverlay.App.Interop;

namespace TaskbarIconOverlay.App.Logging;

public static class Logger
{
    private static bool _isInitialized;

    public static void Initialize()
    {
        if (!_isInitialized)
        {
            NativeLogger.Logger_Init(IntPtr.Zero);
            _isInitialized = true;
        }
    }

    public static void Info(string message)
    {
        if (string.IsNullOrEmpty(message)) return;
        NativeLogger.Logger_Info(message);
    }

    public static void Warn(string message)
    {
        if (string.IsNullOrEmpty(message)) return;
        NativeLogger.Logger_Warn(message);
    }

    public static void Error(string message)
    {
        if (string.IsNullOrEmpty(message)) return;
        NativeLogger.Logger_Error(message);
    }
}
