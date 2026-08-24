using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using TaskbarIconOverlay.App.Models;

namespace TaskbarIconOverlay.App.Converters;

public sealed class NumberPositionToHorizontalAlignmentConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture) =>
        value is NumberPosition pos &&
        (pos == NumberPosition.TopRight || pos == NumberPosition.BottomRight)
            ? HorizontalAlignment.Right
            : HorizontalAlignment.Left;

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture) =>
        throw new NotSupportedException();
}
