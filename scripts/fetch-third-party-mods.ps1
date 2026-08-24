param(
    [string]$SolutionDir = ".\"
)

# Define relative local target path and remote raw URL
$RelativePath = "src/mods/taskbar-grouping/taskbar-grouping.wh.cpp"
$TargetFile   = Join-Path $SolutionDir $RelativePath
$TargetDir    = Split-Path $TargetFile -Parent
$RawUrl       = "https://raw.githubusercontent.com/ramensoftware/windhawk-mods/refs/heads/main/mods/taskbar-grouping.wh.cpp"


if (Test-Path $TargetFile) {
    Write-Host "[Fetch-Mods] File already exists: $RelativePath" -ForegroundColor Green
    exit 0
}

Write-Host "[Fetch-Mods] Missing dependency. Downloading from GitHub..." -ForegroundColor Yellow

# Ensure the destination directory exists
if (-not (Test-Path $TargetDir)) {
    New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
}

try {
    # Set TLS 1.2/1.3 protocol security explicitly for older Windows configurations
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13
    
    # Download the text payload smoothly
    Invoke-WebRequest -Uri $RawUrl -OutFile $TargetFile -UseBasicParsing
    Write-Host "[Fetch-Mods] Successfully downloaded: $RelativePath" -ForegroundColor Green
}
catch {
    Write-Error "[Fetch-Mods] Critical Error: Failed to fetch module dependency from $RawUrl. Exception: $_"
    exit 1
}
