[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$Version
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir   = Split-Path -Parent $ScriptDir

$WindowsVersion = "$Version.0"
$VersionComma  = $WindowsVersion.Replace(".", ",")

$VersionHeader = Join-Path $RootDir "version.h"
$BuildProps    = Join-Path $RootDir "Directory.Build.Version.props"

Write-Host "Setting version to $Version"

# ---------------------------------------------------------------------------
# version.h
# ---------------------------------------------------------------------------

$VersionHeaderContent = @"
#pragma once

#define TIO_VERSION_COMMA $VersionComma
#define TIO_VERSION_STRING "$WindowsVersion"
"@

Set-Content `
    -LiteralPath $VersionHeader `
    -Value $VersionHeaderContent `
    -Encoding UTF8

# ---------------------------------------------------------------------------
# Directory.Build.props
# ---------------------------------------------------------------------------

[xml]$Props = Get-Content -LiteralPath $BuildProps

$PropertyGroup = $Props.Project.PropertyGroup |
    Where-Object {
        $_.PSObject.Properties.Name -contains "Version" -and
        $_.PSObject.Properties.Name -contains "AssemblyVersion" -and
        $_.PSObject.Properties.Name -contains "FileVersion"
    } |
    Select-Object -First 1

if (-not $PropertyGroup) {
    throw "Could not find version PropertyGroup in $BuildProps"
}

$PropertyGroup.AssemblyVersion = $WindowsVersion
$PropertyGroup.FileVersion = $WindowsVersion
$PropertyGroup.Version = $Version

$Props.Save($BuildProps)

Write-Host "Version updated successfully."
