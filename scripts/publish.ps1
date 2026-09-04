[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string]$Version = "0.1.0",

    [Parameter(Mandatory = $false)]
    [ValidateSet("Release")]
    [string]$Configuration = "Release",

    [Parameter(Mandatory = $false)]
    [ValidateSet("x64")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir   = Split-Path -Parent $ScriptDir

$Solution = Join-Path $RootDir "TaskbarIconOverlay.slnx"

$SourceDir = Join-Path $RootDir "src"
$RedistDir = Join-Path $RootDir "redist"

$ArtifactsDir = Join-Path $RootDir "artifacts"
$StageDir     = Join-Path $ArtifactsDir "TaskbarIconOverlay-$Version"
$ZipPath      = Join-Path $ArtifactsDir "TaskbarIconOverlay-$Version.zip"
$HashPath     = Join-Path $ArtifactsDir "TaskbarIconOverlay-$Version.zip.sha256"

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------

function Write-Step {
    param([string]$Message)

    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Assert-FileExists {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required file not found: $Path"
    }
}

function Copy-RequiredFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Source,

        [Parameter(Mandatory = $true)]
        [string]$Destination
    )

    Assert-FileExists $Source

    $DestinationDir = Split-Path -Parent $Destination

    if (-not (Test-Path -LiteralPath $DestinationDir)) {
        New-Item -ItemType Directory -Path $DestinationDir -Force | Out-Null
    }

    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Find-MSBuild {
    $VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

    if (Test-Path -LiteralPath $VsWhere) {
        $MsBuild = & $VsWhere `
            -latest `
            -products * `
            -requires Microsoft.Component.MSBuild `
            -find MSBuild\**\Bin\MSBuild.exe |
            Select-Object -First 1

        if ($MsBuild) {
            return $MsBuild
        }
    }

    $Command = Get-Command msbuild.exe -ErrorAction SilentlyContinue

    if ($Command) {
        return $Command.Source
    }

    throw "MSBuild.exe was not found. Install Visual Studio/MSBuild or run this script from a Developer PowerShell."
}

function Find-DotNet {
    $Command = Get-Command dotnet.exe -ErrorAction SilentlyContinue

    if (-not $Command) {
        throw "dotnet.exe was not found."
    }

    return $Command.Source
}

function Find-ProjectFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProjectName
    )

    $Project = Get-ChildItem `
        -Path $SourceDir `
        -Recurse `
        -Filter "$ProjectName.csproj" |
        Select-Object -First 1

    if (-not $Project) {
        throw "Could not find project: $ProjectName.csproj"
    }

    return $Project.FullName
}

# -----------------------------------------------------------------------------
# Validate repository
# -----------------------------------------------------------------------------

Write-Step "Validating repository"

Assert-FileExists $Solution

if (-not (Test-Path -LiteralPath $RedistDir -PathType Container)) {
    throw "redist directory not found: $RedistDir"
}

$MSBuild = Find-MSBuild
$DotNet  = Find-DotNet

Write-Host "MSBuild: $MSBuild"
Write-Host "dotnet:  $DotNet"
Write-Host "Version: $Version"

# -----------------------------------------------------------------------------
# Clean artifacts
# -----------------------------------------------------------------------------

Write-Step "Cleaning artifacts"

if (Test-Path -LiteralPath $ArtifactsDir) {
    Remove-Item -LiteralPath $ArtifactsDir -Recurse -Force
}

New-Item -ItemType Directory -Path $ArtifactsDir -Force | Out-Null
New-Item -ItemType Directory -Path $StageDir -Force | Out-Null

# -----------------------------------------------------------------------------
# Build solution
# -----------------------------------------------------------------------------

Write-Step "Building solution"

& $MSBuild $Solution `
    /restore `
    /t:Build `
    /p:Configuration=$Configuration `
    /p:Platform=$Platform `
    /m

if ($LASTEXITCODE -ne 0) {
    throw "MSBuild build failed with exit code $LASTEXITCODE."
}

# -----------------------------------------------------------------------------
# Publish WPF application
# -----------------------------------------------------------------------------

Write-Step "Publishing TaskbarIconOverlay.App"

$AppProject = Find-ProjectFile "TaskbarIconOverlay.App"

$AppPublishDir = $StageDir

& $DotNet publish $AppProject `
    -c $Configuration `
    -r win-x64 `
    --self-contained true `
    -o $AppPublishDir

if ($LASTEXITCODE -ne 0) {
    throw "dotnet publish failed with exit code $LASTEXITCODE."
}

# -----------------------------------------------------------------------------
# Locate native build output
#
# The exact intermediate/output layout can vary depending on the Visual Studio
# project configuration. We search the repository instead of hard-coding a
# single bin path.
# -----------------------------------------------------------------------------

Write-Step "Collecting native binaries"

function Find-BuiltBinary {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FileName
    )

    $Candidates = Get-ChildItem `
        -Path "$ArtifactsDir\\bin\\$Configuration\\" `
        -Recurse `
        -Filter $FileName `
        -File |
        Sort-Object LastWriteTime -Descending

    if (-not $Candidates) {
        throw "Could not find built binary: $FileName"
    }

    return $Candidates[0].FullName
}

$EngineDll   = Find-BuiltBinary "TaskbarIconOverlay.Engine.dll"
$InjectorExe = Find-BuiltBinary "TaskbarIconOverlay.Injector.exe"
$OverlayDll  = Find-BuiltBinary "taskbar-icon-overlay.dll"

Copy-RequiredFile `
    $EngineDll `
    (Join-Path $StageDir "TaskbarIconOverlay.Engine.dll")

Copy-RequiredFile `
    $InjectorExe `
    (Join-Path $StageDir "TaskbarIconOverlay.Injector.exe")

Copy-RequiredFile `
    $OverlayDll `
    (Join-Path $StageDir "taskbar-icon-overlay.dll")

# -----------------------------------------------------------------------------
# Third-party taskbar-grouping mod
#
# It is fetched during the build and is intentionally copied as-is.
# Do NOT modify/sign/rebuild it here.
# -----------------------------------------------------------------------------

Write-Step "Collecting taskbar-grouping"

$TaskbarGroupingDll = Find-BuiltBinary "taskbar-grouping.dll"

Copy-RequiredFile `
    $TaskbarGroupingDll `
    (Join-Path $StageDir "taskbar-grouping.dll")

# -----------------------------------------------------------------------------
# Runtime dependencies
# -----------------------------------------------------------------------------

Write-Step "Collecting runtime dependencies"

$RuntimeDependencies = @(
    "TaskbarIconOverlay.Dia.dll",
    "TaskbarIconOverlay.SymSrv.dll"
)

foreach ($FileName in $RuntimeDependencies) {
    Copy-RequiredFile `
        (Join-Path $RedistDir $FileName) `
        (Join-Path $StageDir $FileName)
}

# -----------------------------------------------------------------------------
# Documentation / license
# -----------------------------------------------------------------------------

Write-Step "Collecting release documentation"

$Documentation = @(
    "README.md",
    "LICENSE"
)

foreach ($FileName in $Documentation) {
    $Source = Join-Path $RootDir $FileName

    if (Test-Path -LiteralPath $Source -PathType Leaf) {
        Copy-Item `
            -LiteralPath $Source `
            -Destination (Join-Path $StageDir $FileName) `
            -Force
    }
}

# -----------------------------------------------------------------------------
# Remove development-only files from WPF publish output
# -----------------------------------------------------------------------------

Write-Step "Cleaning publish output"

$DevelopmentFiles = @(
    "*.pdb",
    "*.xml"
)

foreach ($Pattern in $DevelopmentFiles) {
    Get-ChildItem `
        -Path $StageDir `
        -Recurse `
        -File `
        -Filter $Pattern |
        Remove-Item -Force
}

# -----------------------------------------------------------------------------
# Validate release contents
# -----------------------------------------------------------------------------

Write-Step "Validating release package"

$RequiredReleaseFiles = @(
    "TaskbarIconOverlay.App.exe",
    "TaskbarIconOverlay.Engine.dll",
    "TaskbarIconOverlay.Injector.exe",
    "taskbar-icon-overlay.dll",
    "taskbar-grouping.dll",
    "TaskbarIconOverlay.Dia.dll",
    "TaskbarIconOverlay.SymSrv.dll"
)

foreach ($FileName in $RequiredReleaseFiles) {
    Assert-FileExists (Join-Path $StageDir $FileName)
}

Write-Host ""
Write-Host "Release contents:" -ForegroundColor Green

Get-ChildItem `
    -Path $StageDir `
    -Recurse `
    -File |
    ForEach-Object {
        $RelativePath = $_.FullName.Substring($StageDir.Length + 1)
        Write-Host "  $RelativePath"
    }

# -----------------------------------------------------------------------------
# Create ZIP
# -----------------------------------------------------------------------------

Write-Step "Creating release ZIP"

if (Test-Path -LiteralPath $ZipPath) {
    Remove-Item -LiteralPath $ZipPath -Force
}

Compress-Archive `
    -Path (Join-Path $StageDir "*") `
    -DestinationPath $ZipPath `
    -CompressionLevel Optimal

Assert-FileExists $ZipPath

# -----------------------------------------------------------------------------
# SHA-256
# -----------------------------------------------------------------------------

Write-Step "Calculating SHA-256"

$Hash = (Get-FileHash -LiteralPath $ZipPath -Algorithm SHA256).Hash

$Hash | Set-Content `
    -LiteralPath $HashPath `
    -Encoding ASCII

Write-Host "SHA256: $Hash"

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host " Release package created successfully" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Version : $Version"
Write-Host "Package : $ZipPath"
Write-Host "SHA256  : $HashPath"
Write-Host ""