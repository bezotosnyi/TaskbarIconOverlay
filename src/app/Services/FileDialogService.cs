namespace TaskbarIconOverlay.App.Services;

public sealed class FileDialogService : IFileDialogService
{
    public string? BrowseForImage(string? initialDirectory)
    {
        var dialog = new Microsoft.Win32.OpenFileDialog
        {
            Filter = "Images|*.png;*.jpg;*.jpeg;*.bmp;*.ico",
            InitialDirectory = initialDirectory,
        };
        return dialog.ShowDialog() == true ? dialog.FileName : null;
    }
}
