using System.Windows;
using TaskbarIconOverlay.App.Services;
using TaskbarIconOverlay.App.ViewModels;
using TaskbarIconOverlay.App.Views;

namespace TaskbarIconOverlay.App;

public partial class App : Application
{
    private SharedConfigWriter? _configWriter;
    private EngineController? _engineController;

    protected override async void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        _configWriter = new SharedConfigWriter();
        _engineController = new EngineController();
        var fileDialogService = new FileDialogService();
        var viewModel = new MainViewModel(fileDialogService, _configWriter);

        var window = new MainWindow { DataContext = viewModel };
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
        if (_engineController is not null)
        {
            await _engineController.DisableAsync();
        }
        _configWriter?.Dispose();
        base.OnExit(e);
    }
}
