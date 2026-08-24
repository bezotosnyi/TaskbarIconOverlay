using System;
using System.Globalization;
using System.Windows.Data;

namespace TaskbarIconOverlay.App.Converters;

/// <summary>
/// Slider Maximum for NumberedCount: 10 when numbers beyond Win+N
/// aren't allowed, a generous upper bound (matches kMaxIconSlots) otherwise.
/// </summary>
public sealed class BoolToMaxNumberedConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture) =>
        value is true ? Services.SharedConfigWriter.MaxIconSlots : 10;

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture) =>
        throw new NotSupportedException();
}
