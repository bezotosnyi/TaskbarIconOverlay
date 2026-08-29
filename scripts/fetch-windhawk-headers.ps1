param(
    [string]$SolutionDir = ".\"
)

# Pinned Windhawk headers version.
# These headers are fetched from the same pinned windhawk-mods commit
# used as the source of the Windhawk SDK headers.
$PinnedCommit = "d8be922fa99b327d58f6085971c84cfea5863f47"
$BaseUrl      = "https://raw.githubusercontent.com/ramensoftware/windhawk-mods/$PinnedCommit/.vscode/windhawk_headers_1.7.3"

$Headers = @(
    @{
        Name = "windhawk_api.h"
        Url  = "$BaseUrl/windhawk_api.h"
    },
    @{
        Name = "windhawk_utils.h"
        Url  = "$BaseUrl/windhawk_utils.h"
    },
    @{
        Name = "windhawk_api_internal.h"
        Url  = "$BaseUrl/windhawk_api_internal.h"
    }
)

$RelativeDir = "src/wrapper/include"
$TargetDir = Join-Path $SolutionDir $RelativeDir

# Check whether all required headers are already present.
$MissingHeaders = @(
    $Headers | Where-Object {
        -not (Test-Path (Join-Path $TargetDir $_.Name))
    }
)

if ($MissingHeaders.Count -eq 0) {
    Write-Host "[Fetch-Windhawk] All required headers already exist: $RelativeDir" -ForegroundColor Green
    exit 0
}

Write-Host "[Fetch-Windhawk] Missing Windhawk SDK headers. Downloading..." -ForegroundColor Yellow

# Ensure the destination directory exists.
if (-not (Test-Path $TargetDir)) {
    New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
}

try {
    # Set TLS 1.2/1.3 protocol security explicitly for older Windows configurations.
    [Net.ServicePointManager]::SecurityProtocol =
        [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13

    foreach ($Header in $MissingHeaders) {
        $TargetFile = Join-Path $TargetDir $Header.Name

        Write-Host "[Fetch-Windhawk] Downloading $($Header.Name)..."

        Invoke-WebRequest `
            -Uri $Header.Url `
            -OutFile $TargetFile `
            -UseBasicParsing

        Write-Host "[Fetch-Windhawk] Successfully downloaded: $($Header.Name)" -ForegroundColor Green
    }
}
catch {
    Write-Error "[Fetch-Windhawk] Critical Error: Failed to fetch Windhawk SDK headers. Exception: $_"
    exit 1
}
