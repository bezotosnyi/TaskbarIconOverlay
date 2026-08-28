using System.Windows;
using MahApps.Metro.Controls.Dialogs;
using TaskbarIconOverlay.App.Extensions;
using TaskbarIconOverlay.App.Localization;
using TaskbarIconOverlay.App.Services;
using TaskbarIconOverlay.App.Services.Engine;
using TaskbarIconOverlay.App.ViewModels;
using TaskbarIconOverlay.App.Views;

namespace TaskbarIconOverlay.App;

public partial class App : Application
{
    private SettingsPersistenceService? _settingsPersistenceService;
    private SharedConfigWriter? _configWriter;
    private EngineController? _engineController;
    private MainViewModel? _mainViewModel;
    private TrayIconManager? _trayIconManager;

    protected override async void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        _settingsPersistenceService = new SettingsPersistenceService();
        _configWriter = new SharedConfigWriter();
        _engineController = new EngineController();

        var settings = _settingsPersistenceService.Load();
        if (settings is not null)
        {
            LocalizationManager.Instance.SetLanguage(settings.Language);
        }

        var splash = new SplashWindow();
        splash.Show();

        var result = await _engineController.EnableAsync();
        if (result != EngineResult.Enabled)
        {
            var message = LocalizationManager.Instance[
                result.GetLocalizationKey()
            ];

            var dialogSettings = new MetroDialogSettings()
            {
                DialogTitleFontSize = 16,
                DialogMessageFontSize = 14,
                DialogButtonFontSize = 14,
                AnimateShow = true,
                AnimateHide = true
            };
            await splash.ShowMessageAsync(
                LocalizationManager.Instance["WindowTitle"],
                message, MessageDialogStyle.Affirmative, dialogSettings);

            splash.Close();
            Shutdown();
            return;
        }

        splash.Close();

        var fileDialogService = new FileDialogService();
        _mainViewModel = new MainViewModel(fileDialogService, _configWriter, settings);

        var window = new MainWindow { DataContext = _mainViewModel };
        _trayIconManager = new TrayIconManager(window);

        window.Show();
    }

    protected override async void OnExit(ExitEventArgs e)
    {
        if (_engineController is not null)
        {
            var result = await _engineController.DisableAsync();
            if (result != EngineResult.Disabled)
            {
                // TODO: add logging or error handling here
            }
        }

        if (_mainViewModel is not null)
        {
            var settings = _mainViewModel.GetAppSettings();
            _settingsPersistenceService?.Save(settings);
        }

        _trayIconManager?.Dispose();
        _configWriter?.Dispose();
        base.OnExit(e);
    }
}
