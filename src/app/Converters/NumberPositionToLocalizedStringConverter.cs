using System;
using System.Globalization;
using System.Windows.Data;
using TaskbarIconOverlay.App.Localization;
using TaskbarIconOverlay.App.Models;

namespace TaskbarIconOverlay.App.Converters;

public sealed class NumberPositionToLocalizedStringConverter : IMultiValueConverter
{
    public object Convert(
        object[] values,
        Type targetType,
        object? parameter,
        CultureInfo culture)
    {
        if (values.Length == 0 || values[0] is not NumberPosition position)
            return string.Empty;

        var key = position switch
        {
            NumberPosition.TopLeft => "PositionTopLeft",
            NumberPosition.TopRight => "PositionTopRight",
            NumberPosition.BottomLeft => "PositionBottomLeft",
            NumberPosition.BottomRight => "PositionBottomRight",
            _ => throw new ArgumentOutOfRangeException(nameof(position))
        };

        return LocalizationManager.Instance[key];
    }

    public object[] ConvertBack(
        object? value,
        Type[] targetTypes,
        object? parameter,
        CultureInfo culture) =>
        throw new NotSupportedException();
}
