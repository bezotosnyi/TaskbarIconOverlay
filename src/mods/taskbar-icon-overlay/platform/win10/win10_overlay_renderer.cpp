#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <gdiplus.h>

#include <windhawk_utils.h>

#include "core/icon_catalog.h"
#include "core/overlay_configuration.h"
#include "platform/win10/win10_overlay_renderer.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace
{
    using ImagePtr = std::unique_ptr<Gdiplus::Image>;

    ULONG_PTR g_gdiplusToken = 0;
    std::mutex g_mutex;
    std::unordered_map<std::wstring, ImagePtr> g_images;

    Gdiplus::Image* GetImage(const std::wstring& path)
    {
        std::lock_guard lock(g_mutex);
        const auto existing = g_images.find(path);
        if (existing != g_images.end())
            return existing->second.get();

        auto image = std::make_unique<Gdiplus::Image>(path.c_str());
        if (image->GetLastStatus() != Gdiplus::Ok)
        {
            Wh_Log(L"Win10 overlay renderer: couldn't load image '%s'", path.c_str());
            g_images.emplace(path, nullptr);
            return nullptr;
        }

        auto* result = image.get();
        g_images.emplace(path, std::move(image));
        return result;
    }

    void DrawNumber(Gdiplus::Graphics& graphics, const RECT& buttonRect, int position, int dpi)
    {
        const auto& config = OverlayConfiguration::Get();
        if (position < 1 || position > static_cast<int>(config.numberedCount))
            return;

        const std::wstring text = !config.allowNumbersBeyondTen && position == 10
            ? L"0"
            : std::to_wstring(position);

        Gdiplus::FontFamily fontFamily(L"Segoe UI");
        // A pixel-sized GDI+ font renders noticeably smaller than the same
        // FontSize in the Win11 XAML TextBlock. Point units give a comparable
        // visual size and still scale with the target HDC's DPI.
        const float fontSize = static_cast<float>(config.numberSize);
        Gdiplus::Font font(&fontFamily, fontSize, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);

        Gdiplus::RectF measuredBounds;
        graphics.MeasureString(text.c_str(), -1, &font, Gdiplus::PointF{}, &measuredBounds);

        const float marginX = static_cast<float>(MulDiv(4, dpi, USER_DEFAULT_SCREEN_DPI));
        const float marginY = static_cast<float>(MulDiv(2, dpi, USER_DEFAULT_SCREEN_DPI));
        const float left = static_cast<float>(buttonRect.left);
        const float top = static_cast<float>(buttonRect.top);
        const float right = static_cast<float>(buttonRect.right);
        const float bottom = static_cast<float>(buttonRect.bottom);

        float x = left + marginX;
        float y = top + marginY;
        switch (config.numberPosition)
        {
        case SharedConfig::NumberPosition::TopRight:
            x = right - measuredBounds.Width - marginX;
            break;
        case SharedConfig::NumberPosition::BottomLeft:
            y = bottom - measuredBounds.Height - marginY;
            break;
        case SharedConfig::NumberPosition::BottomRight:
            x = right - measuredBounds.Width - marginX;
            y = bottom - measuredBounds.Height - marginY;
            break;
        case SharedConfig::NumberPosition::TopLeft:
        default:
            break;
        }

        const auto foreground = Gdiplus::Color(config.numberColorArgb);
        const auto outline = Gdiplus::Color(config.backgroundColorArgb);
        Gdiplus::SolidBrush outlineBrush(outline);
        Gdiplus::SolidBrush textBrush(foreground);
        for (int offsetX = -1; offsetX <= 1; ++offsetX)
        {
            for (int offsetY = -1; offsetY <= 1; ++offsetY)
            {
                if (offsetX || offsetY)
                    graphics.DrawString(text.c_str(), -1, &font,
                        Gdiplus::PointF(x + offsetX, y + offsetY), &outlineBrush);
            }
        }
        graphics.DrawString(text.c_str(), -1, &font, Gdiplus::PointF(x, y), &textBrush);
    }
}

namespace Win10OverlayRenderer
{
    bool Initialize()
    {
        Gdiplus::GdiplusStartupInput startupInput;
        const auto status = Gdiplus::GdiplusStartup(&g_gdiplusToken, &startupInput, nullptr);
        if (status != Gdiplus::Ok)
        {
            Wh_Log(L"Win10 overlay renderer: GdiplusStartup failed with status %d", static_cast<int>(status));
            g_gdiplusToken = 0;
            return false;
        }

        return true;
    }

    void Shutdown()
    {
        ClearCache();
        if (g_gdiplusToken)
        {
            Gdiplus::GdiplusShutdown(g_gdiplusToken);
            g_gdiplusToken = 0;
        }
    }

    void ClearCache()
    {
        std::lock_guard lock(g_mutex);
        g_images.clear();
    }

    void RefreshTaskbars()
    {
        constexpr UINT redrawFlags = RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN;

        if (HWND primaryTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr))
            RedrawWindow(primaryTaskbar, nullptr, nullptr, redrawFlags);

        HWND secondaryTaskbar = nullptr;
        while ((secondaryTaskbar = FindWindowExW(nullptr, secondaryTaskbar,
            L"Shell_SecondaryTrayWnd", nullptr)))
        {
            RedrawWindow(secondaryTaskbar, nullptr, nullptr, redrawFlags);
        }
    }

    void Draw(HDC hdc, const RECT& buttonRect, int iconPosition, int numberPosition)
    {
        if (!hdc || (iconPosition < 1 && numberPosition < 1))
            return;

        const int buttonWidth = buttonRect.right - buttonRect.left;
        const int buttonHeight = buttonRect.bottom - buttonRect.top;
        if (buttonWidth <= 0 || buttonHeight <= 0)
            return;

        const int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        // Win10's button rect covers the full taskbar slot rather than just
        // the icon panel. A 56 px logical canvas covers the native icon while
        // preserving a small gap around the taskbar button edge.
        Gdiplus::Graphics graphics(hdc);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

        const std::wstring path = IconCatalog::GetPath(iconPosition);
        if (!path.empty())
        {
            if (Gdiplus::Image* image = GetImage(path))
            {
                const int preferredSize = MulDiv(56, dpi > 0 ? dpi : USER_DEFAULT_SCREEN_DPI, USER_DEFAULT_SCREEN_DPI);
                const int availableSize = std::min(buttonWidth, buttonHeight);
                const int targetSize = std::min(preferredSize, availableSize);
                const UINT imageWidth = image->GetWidth();
                const UINT imageHeight = image->GetHeight();
                if (targetSize > 0 && imageWidth && imageHeight)
                {
                    const float scale = std::min(
                        static_cast<float>(targetSize) / imageWidth,
                        static_cast<float>(targetSize) / imageHeight);
                    const float width = imageWidth * scale;
                    const float height = imageHeight * scale;
                    const float left = buttonRect.left + (buttonWidth - width) / 2.0f;
                    const float top = buttonRect.top + (buttonHeight - height) / 2.0f;
                    graphics.DrawImage(image, Gdiplus::RectF(left, top, width, height));
                }
            }
        }

        DrawNumber(graphics, buttonRect, numberPosition, dpi > 0 ? dpi : USER_DEFAULT_SCREEN_DPI);
    }
} // namespace Win10OverlayRenderer
