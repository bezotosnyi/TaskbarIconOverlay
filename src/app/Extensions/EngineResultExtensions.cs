using TaskbarIconOverlay.App.Services.Engine;

namespace TaskbarIconOverlay.App.Extensions;

public static class EngineResultExtensions
{
    public static string GetLocalizationKey(this EngineResult result)
    {
        return result switch
        {
            EngineResult.ExplorerNotFound =>
                "EngineExplorerNotFound",

            EngineResult.OpenProcessFailed =>
                "EngineOpenProcessFailed",

            EngineResult.WindhawkConflict =>
                "EngineWindhawkConflict",

            EngineResult.EngineDllNotFound =>
                "EngineDllNotFound",

            EngineResult.InjectionFailed =>
                "EngineInjectionFailed",

            EngineResult.RemoteExportCallFailed =>
                "EngineRemoteExportCallFailed",

            EngineResult.UsageError =>
                "EngineUsageError",

            _ =>
                "EngineUnknownError"
        };
    }
}
