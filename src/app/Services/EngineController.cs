using System;
using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;

namespace TaskbarIconOverlay.App.Services;

public sealed class EngineController
{
    private readonly string _injectorPath;

    public EngineController()
    {
        _injectorPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory,
            "TaskbarIconOverlay.Injector.exe");
    }

    public Task<bool> EnableAsync() => RunAsync("enable");
    public Task<bool> DisableAsync() => RunAsync("disable");

    private async Task<bool> RunAsync(string command)
    {
        var psi = new ProcessStartInfo(_injectorPath, command)
        {
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true,
        };

        using var process = Process.Start(psi);
        if (process is null) return false;

        await process.WaitForExitAsync();
        return process.ExitCode == 0;
    }
}
