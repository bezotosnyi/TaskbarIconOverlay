using System.Runtime.InteropServices;

namespace TaskbarIconOverlay.App.Interop;

internal static partial class NativeLogger
{
    private const string DllName = "TaskbarIconOverlay.Logger.Interop.dll";

    [LibraryImport(DllName)]
    internal static partial void Logger_Init(nint ownModule);

    [LibraryImport(DllName, StringMarshalling = StringMarshalling.Utf16)]
    internal static partial void Logger_Info(string text);

    [LibraryImport(DllName, StringMarshalling = StringMarshalling.Utf16)]
    internal static partial void Logger_Warn(string text);

    [LibraryImport(DllName, StringMarshalling = StringMarshalling.Utf16)]
    internal static partial void Logger_Error(string text);
}
