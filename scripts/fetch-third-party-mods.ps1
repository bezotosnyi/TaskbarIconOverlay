param(
    [string]$SolutionDir = ".\"
)

# Define relative local target path and remote raw URL
$RelativePath = "src/mods/taskbar-grouping/taskbar-grouping.wh.cpp"
$TargetFile   = Join-Path $SolutionDir $RelativePath
$TargetDir    = Split-Path $TargetFile -Parent
$PinnedCommit = "be471af902a3370c41251653153ce4d94c170b81"
$PinnedVersion = "1.3.12"
$VersionPattern = "(?m)^\s*//\s*@version\s+$([regex]::Escape($PinnedVersion))\s*$"
$RawUrl       = "https://raw.githubusercontent.com/ramensoftware/windhawk-mods/$PinnedCommit/mods/taskbar-grouping.wh.cpp"


if (Test-Path $TargetFile) {
    if (Select-String -Path $TargetFile -Pattern $VersionPattern -Quiet) {
        Write-Host "[Fetch-Mods] Dependency is current: $RelativePath (v$PinnedVersion)" -ForegroundColor Green
        exit 0
    }

    Write-Host "[Fetch-Mods] Existing dependency is outdated. Updating to v$PinnedVersion..." -ForegroundColor Yellow
} else {
    Write-Host "[Fetch-Mods] Missing dependency. Downloading from GitHub..." -ForegroundColor Yellow
}

# Ensure the destination directory exists
if (-not (Test-Path $TargetDir)) {
    New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
}

$TemporaryFile = "$TargetFile.$PID.download"

try {
    # Set TLS 1.2/1.3 protocol security explicitly for older Windows configurations
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13
    
    Invoke-WebRequest -Uri $RawUrl -OutFile $TemporaryFile -UseBasicParsing

    if (-not (Select-String -Path $TemporaryFile -Pattern $VersionPattern -Quiet)) {
        throw "Downloaded source does not declare the expected version v$PinnedVersion."
    }

    Move-Item -LiteralPath $TemporaryFile -Destination $TargetFile -Force
    Write-Host "[Fetch-Mods] Successfully updated: $RelativePath (v$PinnedVersion)" -ForegroundColor Green
}
catch {
    if (Test-Path $TemporaryFile) {
        Remove-Item -LiteralPath $TemporaryFile -Force
    }

    Write-Error "[Fetch-Mods] Critical Error: Failed to fetch module dependency from $RawUrl. Exception: $_"
    exit 1
}
