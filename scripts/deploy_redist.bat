@echo off
set "SRC=%~1"
set "DST=%~2"

:: Check if source directory exists
if not exist "%SRC%" (
    echo [Redist] Source directory does not exist: %SRC%
    exit /b 0
)

:: Run robocopy: 
:: /E   = Copy subdirectories, including empty ones
:: /XO  = Exclude Older files (skips files if the destination is newer/identical)
:: /PURGE = Optional: Deletes files in output folder that no longer exist in redist
robocopy "%SRC%" "%DST%" /E /XO /NJH /NJS /NDL /NC

:: Robocopy returns codes 0-7 for various success/no-change states. 
:: Codes 8 or higher mean actual errors occurred.
if %errorlevel% GEQ 8 (
    echo [Redist] Copy failed with error level %errorlevel%
    exit /b 1
)

echo [Redist] Sync complete.
exit /b 0
