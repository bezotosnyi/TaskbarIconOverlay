using System;
using System.Threading;
using MahApps.Metro.Controls.Dialogs;
using System.Threading.Tasks;
using System.Windows;
using TaskbarIconOverlay.App.Extensions;
using TaskbarIconOverlay.App.Localization;
using TaskbarIconOverlay.App.Logging;
using TaskbarIconOverlay.App.Services;
using TaskbarIconOverlay.App.Services.Engine;
using TaskbarIconOverlay.App.ViewModels;
using TaskbarIconOverlay.App.Views;

namespace TaskbarIconOverlay.App;

public partial class App : Application
{
    private const string InstanceMutexName = "Local\\TaskbarIconOverlay_SingleInstance";

    private SettingsPersistenceService? _settingsPersistenceService;
    private SharedConfigWriter? _configWriter;
    private EngineController? _engineController;
    private MainViewModel? _mainViewModel;
    private TrayIconManager? _trayIconManager;

    private Mutex? _instanceMutex;
    private bool _shutdownStarted;

    protected override async void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        Logger.Initialize();
        try
        {
            _instanceMutex = new Mutex(
                initiallyOwned: true,
                name: InstanceMutexName,
                createdNew: out var createdNew);

            if (!createdNew)
            {
                _instanceMutex.Dispose();
                _instanceMutex = null;

                Logger.Warn("Another instance of the application is already running");

                Shutdown();
                return;
            }

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

                Logger.Error($"Failed to enable the engine: {message}");

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
        catch (Exception exception)
        {
            Logger.Error($"Unhandled exception during startup: {exception.Message}");
            Shutdown();
        }
    }

    protected override async void OnExit(ExitEventArgs e)
    {
        await CleanupAsync();
        base.OnExit(e);
    }

    protected override async void OnSessionEnding(SessionEndingCancelEventArgs e)
    {
        await CleanupAsync();
        base.OnSessionEnding(e);
    }

    private async Task CleanupAsync()
    {
        if (_shutdownStarted)
        {
            return;
        }

        _shutdownStarted = true;

        try
        {
            if (_mainViewModel is not null)
            {
                var settings = _mainViewModel.GetAppSettings();
                _settingsPersistenceService?.Save(settings);
            }

            if (_engineController is not null)
            {
                var result = await _engineController.DisableAsync();
                if (result != EngineResult.Disabled)
                {
                    Logger.Error($"Failed to disable the engine: {LocalizationManager.Instance[result.GetLocalizationKey()]}");
                }
            }
        }
        finally
        {
            _trayIconManager?.Dispose();
            _configWriter?.Dispose();
            _instanceMutex?.Dispose();
        }
    }
}
