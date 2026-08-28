using System;
using System.IO;
using System.Text.Json;
using TaskbarIconOverlay.App.Models;

namespace TaskbarIconOverlay.App.Services;

public sealed class SettingsPersistenceService
{
    private static readonly string FilePath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "TaskbarIconOverlay", "app-settings.json");

    public AppSettings? Load()
    {
        try
        {
            if (File.Exists(FilePath))
            {
                var json = File.ReadAllText(FilePath);
                return JsonSerializer.Deserialize<AppSettings>(json) ?? new AppSettings();
            }
        }
        catch
        {
            // Corrupted or unreadable file - fall back to defaults rather
            // than crash the app on startup.
        }

        return null;
    }

    public void Save(AppSettings data)
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
            var json = JsonSerializer.Serialize(data, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(FilePath, json);
        }
        catch
        {
            // Best-effort - a failed save shouldn't crash the app either.
        }
    }
}
