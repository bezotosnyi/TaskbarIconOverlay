using System;
using System.ComponentModel;
using System.Globalization;

namespace TaskbarIconOverlay.App.Localization;

public sealed class LocalizationManager : INotifyPropertyChanged
{
    private static readonly Lazy<LocalizationManager> _lazy = new(() => new LocalizationManager());
    public static LocalizationManager Instance => _lazy.Value;

    private LocalizationManager()
    {
        // Detect the current language and culture based on the system settings
        CurrentLanguage = DetectLanguage();
        CurrentCulture = GetCulture(CurrentLanguage);

        CultureInfo.CurrentUICulture = CurrentCulture;
    }

    public string this[string key] =>
        Resources.Strings.ResourceManager.GetString(key, CurrentCulture) ?? key;

    public AppLanguage CurrentLanguage { get; private set; }
    public CultureInfo CurrentCulture { get; private set; }

    public void SetLanguage(AppLanguage language)
    {
        if (CurrentLanguage == language)
        {
            return;
        }

        CurrentLanguage = language;
        CurrentCulture = GetCulture(CurrentLanguage);
        CultureInfo.CurrentUICulture = CurrentCulture;

        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Item[]"));
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(CurrentLanguage)));
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(CurrentCulture)));
        LanguageChanged?.Invoke(this, language);
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    public event EventHandler<AppLanguage>? LanguageChanged;

    private static AppLanguage DetectLanguage()
    {
        return CultureInfo.CurrentUICulture.TwoLetterISOLanguageName switch
        {
            "uk" => AppLanguage.Ukrainian,
            "ru" => AppLanguage.Russian,
            "en" => AppLanguage.English,
            _ => AppLanguage.English
        };
    }

    private static CultureInfo GetCulture(AppLanguage language)
    {
        return language switch
        {
            AppLanguage.English => CultureInfo.GetCultureInfo("en-US"),
            AppLanguage.Ukrainian => CultureInfo.GetCultureInfo("uk-UA"),
            AppLanguage.Russian => CultureInfo.GetCultureInfo("ru-RU"),
            _ => throw new ArgumentOutOfRangeException(nameof(language))
        };
    }
}
