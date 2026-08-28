namespace TaskbarIconOverlay.App.Services.Engine;

public enum EngineResult
{
    Enabled = 0,
    Disabled = 1,

    UsageError = 2,

    ExplorerNotFound = 10,
    OpenProcessFailed = 11,
    WindhawkConflict = 12,
    EngineDllNotFound = 13,
    InjectionFailed = 14,
    RemoteExportCallFailed = 15
}
