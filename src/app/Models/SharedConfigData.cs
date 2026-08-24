using System;
using System.Collections.Generic;

namespace TaskbarIconOverlay.App.Models;

/// <summary>
/// Plain data carrier passed to SharedConfigWriter.Write - kept
/// separate from the ViewModel so the writer doesn't depend on WPF types.
/// </summary>
public sealed class SharedConfigData
{
    public IReadOnlyList<string> IconPaths { get; init; } = Array.Empty<string>();
    public bool StickyIconBinding { get; init; }
    public int NumberedCount { get; init; }
    public bool AllowNumbersBeyondTen { get; init; }
    public NumberPosition NumberPosition { get; init; }
    public int NumberSize { get; init; }
    public uint NumberColorArgb { get; init; }
    public uint BackgroundColorArgb { get; init; }
    public bool ShowOnAllTaskbars { get; init; }
}
