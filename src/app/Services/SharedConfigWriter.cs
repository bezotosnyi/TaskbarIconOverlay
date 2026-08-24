using System;
using System.IO.MemoryMappedFiles;
using System.Text;
using System.Threading;
using TaskbarIconOverlay.App.Models;

namespace TaskbarIconOverlay.App.Services;

/// <summary>
/// Writes the IPC contract read by taskbar-icon-overlay.dll. Layout is
/// duplicated by hand in the mod's own shared_config.h (no shared project,
/// by design) - if you change field order/size here, update that file too.
///
/// Layout (all integers little-endian, matches C++ struct with
/// #pragma pack(push, 1)):
///   uint32 version
///   uint32 windowCount
///   wchar_t iconPaths[50][260]     (each slot padded to 260 wchar_t)
///   uint32 stickyIconBinding       (bool as uint32)
///   uint32 numberedCount
///   uint32 allowNumbersBeyondTen   (bool as uint32)
///   uint32 numberPosition          (0=TopLeft,1=TopRight,2=BottomLeft,3=BottomRight)
///   uint32 numberSize
///   uint32 numberColorArgb
///   uint32 backgroundColorArgb
///   uint32 showOnAllTaskbars       (bool as uint32)
/// </summary>
public sealed class SharedConfigWriter : IDisposable
{
    public const int MaxIconSlots = 50;
    private const int MaxPathChars = 260;          // MAX_PATH
    private const int IconSlotBytes = MaxPathChars * 2;  // wchar_t = 2 bytes

    private const int VersionOffset = 0;
    private const int WindowCountOffset = 4;
    private const int IconPathsOffset = 8;
    private const int StickyIconBindingOffset = IconPathsOffset + MaxIconSlots * IconSlotBytes;
    private const int NumberedCountOffset = StickyIconBindingOffset + 4;
    private const int AllowNumbersBeyondTenOffset = NumberedCountOffset + 4;
    private const int NumberPositionOffset = AllowNumbersBeyondTenOffset + 4;
    private const int NumberSizeOffset = NumberPositionOffset + 4;
    private const int NumberColorArgbOffset = NumberSizeOffset + 4;
    private const int BackgroundColorArgbOffset = NumberColorArgbOffset + 4;
    private const int ShowOnAllTaskbarsOffset = BackgroundColorArgbOffset + 4;
    private const int TotalBytes = ShowOnAllTaskbarsOffset + 4;

    private const string MemName = "Local\\TaskbarIconOverlay_Config";
    private const string EnabledEventName = "Local\\TaskbarIconOverlay_Enabled";
    private const string ConfigChangedEventName = "Local\\TaskbarIconOverlay_ConfigChanged";

    private readonly MemoryMappedFile _mmf;
    private readonly MemoryMappedViewAccessor _accessor;
    private readonly EventWaitHandle _enabledEvent;
    private readonly EventWaitHandle _configChangedEvent;

    public SharedConfigWriter()
    {
        _mmf = MemoryMappedFile.CreateOrOpen(MemName, TotalBytes);
        _accessor = _mmf.CreateViewAccessor(0, TotalBytes);

        // ManualReset: the signaled state itself IS the enabled/disabled
        // flag - the mod does a non-blocking WaitForSingleObject(hEvent, 0).
        _enabledEvent = new EventWaitHandle(false, EventResetMode.ManualReset, EnabledEventName);

        // AutoReset: each Set() is a one-shot "pulse" for the mod's watcher
        // thread, which waits with a timeout and doesn't reset it manually.
        _configChangedEvent = new EventWaitHandle(false, EventResetMode.AutoReset, ConfigChangedEventName);
    }

    public void Write(SharedConfigData data)
    {
        _accessor.Write(VersionOffset, (uint)1);
        _accessor.Write(WindowCountOffset, (uint)Math.Min(data.IconPaths.Count, MaxIconSlots));

        for (var i = 0; i < MaxIconSlots; i++)
        {
            var buf = new byte[IconSlotBytes];  // zero-filled = empty string by default
            if (i < data.IconPaths.Count && !string.IsNullOrEmpty(data.IconPaths[i]))
            {
                var encoded = Encoding.Unicode.GetBytes(data.IconPaths[i]);
                var copyLen = Math.Min(encoded.Length, IconSlotBytes - 2);  // leave room for L'\0'
                Array.Copy(encoded, buf, copyLen);
            }
            _accessor.WriteArray(IconPathsOffset + i * IconSlotBytes, buf, 0, buf.Length);
        }

        _accessor.Write(StickyIconBindingOffset, (uint)(data.StickyIconBinding ? 1 : 0));
        _accessor.Write(NumberedCountOffset, (uint)data.NumberedCount);
        _accessor.Write(AllowNumbersBeyondTenOffset, (uint)(data.AllowNumbersBeyondTen ? 1 : 0));
        _accessor.Write(NumberPositionOffset, (uint)data.NumberPosition);
        _accessor.Write(NumberSizeOffset, (uint)data.NumberSize);
        _accessor.Write(NumberColorArgbOffset, data.NumberColorArgb);
        _accessor.Write(BackgroundColorArgbOffset, data.BackgroundColorArgb);
        _accessor.Write(ShowOnAllTaskbarsOffset, (uint)(data.ShowOnAllTaskbars ? 1 : 0));

        _configChangedEvent.Set();
    }

    public void SetEnabled(bool enabled)
    {
        if (enabled) _enabledEvent.Set();
        else _enabledEvent.Reset();

        _configChangedEvent.Set();  // force an immediate redraw in the mod
    }

    public void Dispose()
    {
        _accessor.Dispose();
        _mmf.Dispose();
        _enabledEvent.Dispose();
        _configChangedEvent.Dispose();
    }
}
