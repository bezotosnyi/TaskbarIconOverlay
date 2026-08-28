using System.Windows.Media;

namespace TaskbarIconOverlay.App.Extensions;

public static class ColorExtensions
{
    public static string ToHex(this Color color) =>
        $"#{color.A:X2}{color.R:X2}{color.G:X2}{color.B:X2}";

    public static Color ToColor(this string hex, Color fallback)
    {
        try
        {
            return (Color)ColorConverter.ConvertFromString(hex)!;
        }
        catch
        {
            return fallback;
        }
    }
}
