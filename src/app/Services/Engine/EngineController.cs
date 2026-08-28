using System;
using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;

namespace TaskbarIconOverlay.App.Services.Engine;

public sealed class EngineController
{
    private readonly string _injectorPath;

    public EngineController()
    {
        _injectorPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory,
            "TaskbarIconOverlay.Injector.exe");
    }

    public Task<EngineResult> EnableAsync() => RunAsync("enable");
    public Task<EngineResult> DisableAsync() => RunAsync("disable");

    private async Task<EngineResult> RunAsync(string command)
    {
        var psi = new ProcessStartInfo(_injectorPath, command)
        {
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true,
        };

        using var process = Process.Start(psi);
        if (process is null) return EngineResult.OpenProcessFailed;

        await process.WaitForExitAsync();

        return Enum.IsDefined(typeof(EngineResult), process.ExitCode)
            ? (EngineResult)process.ExitCode
            : EngineResult.UsageError;
    }
}
