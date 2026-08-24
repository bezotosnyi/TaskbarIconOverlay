namespace TaskbarIconOverlay.App.Services;

public interface IFileDialogService
{
    /// <returns>Selected file path, or null if the user cancelled.</returns>
    string? BrowseForImage(string? initialDirectory);
}
