using System.Windows;
using TaskbarIconOverlay.App.Localization;
using TaskbarIconOverlay.App.Services;
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

        var fileDialogService = new FileDialogService();
        _mainViewModel = new MainViewModel(fileDialogService, _configWriter, settings);

        var window = new MainWindow { DataContext = _mainViewModel };
        _trayIconManager = new TrayIconManager(window);

        window.Show();

        var injected = await _engineController.EnableAsync();
        if (!injected)
        {
            MessageBox.Show(window,
                "Не вдалось ін'єктувати рушій у explorer.exe. Перевірте " +
                "TaskbarIconOverlay.log поруч із програмою.",
                "TaskbarIconOverlay", MessageBoxButton.OK, MessageBoxImage.Warning);
        }
    }

    protected override async void OnExit(ExitEventArgs e)
    {
        if (_mainViewModel is not null)
        {
            var settings = _mainViewModel.GetAppSettings();
            _settingsPersistenceService?.Save(settings);
        }

        if (_engineController is not null)
        {
            await _engineController.DisableAsync();
        }

        _trayIconManager?.Dispose();
        _configWriter?.Dispose();
        base.OnExit(e);
    }
}
