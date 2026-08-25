using System;
using System.Windows.Markup;

using static TaskbarIconOverlay.App.Resources.Strings;

namespace TaskbarIconOverlay.App.Localization;

/// <summary>
/// Usage: Content="{loc:Loc ToggleEnable}" - resolves via
/// Strings.resx/Strings.uk.resx/Strings.ru.resx based on
/// Thread.CurrentThread.CurrentUICulture at the moment the binding is
/// evaluated. Set CurrentUICulture once at startup (App.xaml.cs, before
/// any window is constructed) to switch language app-wide.
/// </summary>
public sealed class LocalizationExtensions : MarkupExtension
{
    public string Key { get; set; }

    public LocalizationExtensions() { Key = ""; }
    public LocalizationExtensions(string key) { Key = key; }

    public override object ProvideValue(IServiceProvider serviceProvider)
    {
        return ResourceManager.GetString(
            Key, System.Threading.Thread.CurrentThread.CurrentUICulture) ?? Key;
    }
}
