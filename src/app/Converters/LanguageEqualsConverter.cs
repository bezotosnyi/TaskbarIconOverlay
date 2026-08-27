using System;
using System.Globalization;
using System.Windows.Data;
using TaskbarIconOverlay.App.Localization;

namespace TaskbarIconOverlay.App.Converters;

public sealed class LanguageEqualsConverter : IValueConverter
{
    public object Convert(
        object? value,
        Type targetType,
        object? parameter,
        CultureInfo culture)
    {
        return value is AppLanguage currentLanguage &&
               parameter is AppLanguage language &&
               currentLanguage == language;
    }

    public object ConvertBack(
        object? value,
        Type targetType,
        object? parameter,
        CultureInfo culture)
    {
        return Binding.DoNothing;
    }
}
