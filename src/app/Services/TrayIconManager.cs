using System;
using System.Windows;
using System.Windows.Forms;
using TaskbarIconOverlay.App.Localization;
using Application = System.Windows.Application;

namespace TaskbarIconOverlay.App.Services;

public sealed class TrayIconManager : IDisposable
{
    private readonly NotifyIcon _notifyIcon;
    private readonly Window _mainWindow;
    private readonly ContextMenuStrip _menu;
    private readonly ToolStripMenuItem _showItem;
    private readonly ToolStripMenuItem _exitItem;

    public TrayIconManager(Window mainWindow)
    {
        _mainWindow = mainWindow;

        _showItem = new ToolStripMenuItem();
        _showItem.Click += (_, _) => ShowMainWindow();

        _exitItem = new ToolStripMenuItem();
        _exitItem.Click += (_, _) => Application.Current.Shutdown();

        _menu = new ContextMenuStrip();
        _menu.Items.Add(_showItem);
        _menu.Items.Add(_exitItem);

        _notifyIcon = new NotifyIcon
        {
            Icon = System.Drawing.SystemIcons.Application,
            Visible = true,
            ContextMenuStrip = _menu
        };

        _notifyIcon.DoubleClick += (_, _) => ShowMainWindow();

        LocalizationManager.Instance.LanguageChanged += OnLanguageChanged;

        UpdateLocalization();
    }

    private void OnLanguageChanged(object? sender, AppLanguage language)
    {
        UpdateLocalization();
    }

    private void UpdateLocalization()
    {
        _notifyIcon.Text =
            LocalizationManager.Instance["TrayTooltip"];

        _showItem.Text =
            LocalizationManager.Instance["TrayShow"];

        _exitItem.Text =
            LocalizationManager.Instance["TrayExit"];
    }

    private void ShowMainWindow()
    {
        _mainWindow.WindowState = WindowState.Normal;
        _mainWindow.Show();
        _mainWindow.Activate();
    }

    public void Dispose()
    {
        LocalizationManager.Instance.LanguageChanged -= OnLanguageChanged;

        _notifyIcon.Visible = false;
        _notifyIcon.Dispose();
        _menu.Dispose();
    }
}
