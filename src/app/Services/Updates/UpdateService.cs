using System;
using System.Diagnostics;
using System.IO;
using System.Net.Http;
using System.Reflection;
using System.Security.Cryptography;
using System.Text.Json;
using System.Threading.Tasks;

namespace TaskbarIconOverlay.App.Services.Updates;

public sealed class UpdateService
{
    private const string LatestReleaseUrl =
        "https://api.github.com/repos/bezotosnyi/TaskbarIconOverlay/releases/latest";
    private const string InstallerAssetName = "TaskbarIconOverlay-Setup-x64.exe";

    private static readonly HttpClient HttpClient = CreateHttpClient();

    public async Task<UpdateCheckResult?> CheckAsync()
    {
        using var response = await HttpClient.GetAsync(LatestReleaseUrl).ConfigureAwait(false);
        if (!response.IsSuccessStatusCode)
        {
            return null; // No published release yet, offline, or GitHub unavailable.
        }

        await using var stream = await response.Content.ReadAsStreamAsync().ConfigureAwait(false);
        using var document = await JsonDocument.ParseAsync(stream).ConfigureAwait(false);
        var root = document.RootElement;

        var tag = root.GetProperty("tag_name").GetString() ?? string.Empty;
        if (!Version.TryParse(tag.TrimStart('v', 'V'), out var latestVersion))
        {
            return null;
        }

        var currentVersion = Assembly.GetExecutingAssembly().GetName().Version ?? new Version(0, 0);
        if (latestVersion.CompareTo(currentVersion) <= 0)
        {
            return null;
        }

        Uri? installerUri = null;
        string? installerSha256 = null;
        foreach (var asset in root.GetProperty("assets").EnumerateArray())
        {
            var name = asset.GetProperty("name").GetString();
            if (!string.Equals(name, InstallerAssetName, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            var url = asset.GetProperty("browser_download_url").GetString();
            if (Uri.TryCreate(url, UriKind.Absolute, out var parsed))
            {
                installerUri = parsed;
            }

            var digest = asset.TryGetProperty("digest", out var digestElement)
                ? digestElement.GetString()
                : null;
            if (!string.IsNullOrWhiteSpace(digest) && digest.StartsWith("sha256:", StringComparison.OrdinalIgnoreCase))
            {
                installerSha256 = digest["sha256:".Length..];
            }
            break;
        }

        return new UpdateCheckResult(
            currentVersion,
            latestVersion,
            root.GetProperty("html_url").GetString() ?? "https://github.com/bezotosnyi/TaskbarIconOverlay/releases",
            installerUri,
            installerSha256);
    }

    public async Task<string> DownloadInstallerAsync(UpdateCheckResult update)
    {
        if (update.InstallerUri is null)
        {
            throw new InvalidOperationException("The latest release does not include an x64 installer.");
        }

        var updatesDirectory = Path.Combine(Path.GetTempPath(), "TaskbarIconOverlay", "updates");
        Directory.CreateDirectory(updatesDirectory);
        var installerPath = Path.Combine(updatesDirectory, InstallerAssetName);

        await using (var source = await HttpClient.GetStreamAsync(update.InstallerUri).ConfigureAwait(false))
        await using (var destination = File.Create(installerPath))
        {
            await source.CopyToAsync(destination).ConfigureAwait(false);
        }

        if (!string.IsNullOrWhiteSpace(update.InstallerSha256))
        {
            await using var file = File.OpenRead(installerPath);
            var hash = Convert.ToHexString(await SHA256.HashDataAsync(file).ConfigureAwait(false));
            if (!hash.Equals(update.InstallerSha256, StringComparison.OrdinalIgnoreCase))
            {
                File.Delete(installerPath);
                throw new InvalidDataException("The downloaded installer failed its SHA-256 verification.");
            }
        }

        return installerPath;
    }

    public static void LaunchInstaller(string installerPath) =>
        Process.Start(new ProcessStartInfo(installerPath)
        {
            UseShellExecute = true,
            Arguments = "/CLOSEAPPLICATIONS"
        });

    public static void OpenReleasePage(string releasePageUrl) =>
        Process.Start(new ProcessStartInfo(releasePageUrl) { UseShellExecute = true });

    private static HttpClient CreateHttpClient()
    {
        var client = new HttpClient { Timeout = TimeSpan.FromSeconds(15) };
        client.DefaultRequestHeaders.UserAgent.ParseAdd("TaskbarIconOverlay-Updater");
        client.DefaultRequestHeaders.Accept.ParseAdd("application/vnd.github+json");
        client.DefaultRequestHeaders.Add("X-GitHub-Api-Version", "2026-03-10");
        return client;
    }
}
