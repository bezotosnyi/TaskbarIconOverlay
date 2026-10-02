#include "win11_visual_tree.h"

#include <winrt/Windows.UI.Xaml.Media.h>

#include <functional>

namespace
{
    using namespace winrt::Windows::UI::Xaml;
    using namespace winrt::Windows::UI::Xaml::Media;

    FrameworkElement EnumerateChildren(
        const FrameworkElement& element,
        const std::function<bool(FrameworkElement)>& callback)
    {
        const int childrenCount = VisualTreeHelper::GetChildrenCount(element);
        for (int index = 0; index < childrenCount; ++index)
        {
            const auto child = VisualTreeHelper::GetChild(element, index).try_as<FrameworkElement>();
            if (child && callback(child))
                return child;
        }
        return nullptr;
    }
}

namespace Win11VisualTree
{
    FrameworkElement FindChildByName(const FrameworkElement& element, const wchar_t* name)
    {
        if (!element)
            return nullptr;

        try
        {
            return EnumerateChildren(element, [name](const FrameworkElement& child) {
                return child.Name() == name;
            });
        }
        catch (...)
        {
            return nullptr;
        }
    }

    FrameworkElement FindChildByClassName(const FrameworkElement& element, const wchar_t* className)
    {
        return EnumerateChildren(element, [className](const FrameworkElement& child) {
            return winrt::get_class_name(child) == className;
        });
    }

    bool IsSecondaryTaskbar(const XamlRoot& xamlRoot)
    {
        FrameworkElement child = xamlRoot.Content().try_as<FrameworkElement>();
        if (!child ||
            !(child = FindChildByClassName(child, L"SystemTray.SystemTrayFrame")) ||
            !(child = FindChildByName(child, L"SystemTrayFrameGrid")) ||
            !(child = FindChildByName(child, L"ControlCenterButton")))
        {
            return false;
        }

        return child.ActualWidth() < 5;
    }
} // namespace Win11VisualTree
