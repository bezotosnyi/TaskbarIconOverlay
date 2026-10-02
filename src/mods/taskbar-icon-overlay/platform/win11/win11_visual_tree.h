#pragma once

#include <winrt/Windows.UI.Xaml.h>

namespace Win11VisualTree
{
    using FrameworkElement = winrt::Windows::UI::Xaml::FrameworkElement;
    using XamlRoot = winrt::Windows::UI::Xaml::XamlRoot;

    FrameworkElement FindChildByName(const FrameworkElement& element, const wchar_t* name);
    FrameworkElement FindChildByClassName(const FrameworkElement& element, const wchar_t* className);
    bool IsSecondaryTaskbar(const XamlRoot& xamlRoot);
} // namespace Win11VisualTree
