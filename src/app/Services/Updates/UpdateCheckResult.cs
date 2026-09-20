using System;

namespace TaskbarIconOverlay.App.Services.Updates;

public sealed record UpdateCheckResult(
    Version CurrentVersion,
    Version LatestVersion,
    string ReleasePageUrl,
    Uri? InstallerUri,
    string? InstallerSha256);
