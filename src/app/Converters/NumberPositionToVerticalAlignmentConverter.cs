using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using TaskbarIconOverlay.App.Models;

namespace TaskbarIconOverlay.App.Converters;

public sealed class NumberPositionToVerticalAlignmentConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture) =>
        value is NumberPosition pos &&
        (pos == NumberPosition.BottomLeft || pos == NumberPosition.BottomRight)
            ? VerticalAlignment.Bottom
            : VerticalAlignment.Top;

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture) =>
        throw new NotSupportedException();
}
