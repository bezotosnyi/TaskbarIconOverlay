using System.Collections.Generic;
using TaskbarIconOverlay.App.Localization;

namespace TaskbarIconOverlay.App.Models;

public sealed class AppSettings
{
    public List<string> IconPaths { get; set; } = new() { "" };  // default: 1 empty slot
    public bool StickyIconBinding { get; set; } = true;
    public int NumberedCount { get; set; } = 10;
    public bool AllowNumbersBeyondTen { get; set; }
    public NumberPosition NumberPosition { get; set; } = NumberPosition.TopLeft;
    public int NumberSize { get; set; } = 12;
    public string NumberColorHex { get; set; } = "#FFFFFFFF";
    public string BackgroundColorHex { get; set; } = "#80000000";
    public bool ShowOnAllTaskbars { get; set; }
    public AppLanguage Language { get; set; } = AppLanguage.English;
}
