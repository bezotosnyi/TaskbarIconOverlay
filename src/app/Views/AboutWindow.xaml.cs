using System;
using System.Diagnostics;
using System.Reflection;
using System.Windows;
using System.Windows.Navigation;

namespace TaskbarIconOverlay.App.Views;

public partial class AboutWindow
{
    public string VersionText { get; } =
        $"Version {Assembly.GetExecutingAssembly().GetName().Version}";

    public AboutWindow()
    {
        InitializeComponent();
        DataContext = this;
    }

    private void CloseButton_Click(object sender, RoutedEventArgs e) => Close();

    private void RepositoryLink_RequestNavigate(object sender, RequestNavigateEventArgs e)
    {
        Process.Start(new ProcessStartInfo(e.Uri.AbsoluteUri) { UseShellExecute = true });
        e.Handled = true;
    }
}
