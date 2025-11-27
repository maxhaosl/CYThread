@echo off
setlocal enableextensions enabledelayedexpansion

rem Build all Windows-supported targets (MSVC + Android via Bash helper).

set "SCRIPT_DIR=%~dp0"
if not defined SCRIPT_DIR (
    echo [all] Failed to determine script directory
    exit /b 1
)

call "%SCRIPT_DIR%build_windows.bat"
if errorlevel 1 goto :error

call :build_android
if errorlevel 1 goto :error

echo [all] All builds finished
exit /b 0

:build_android
for /f "delims=" %%I in ('where bash 2^>nul') do (
    set "BASH_EXE=%%~fI"
    goto :bash_found
)
echo [all] bash.exe not found; skipping Android build
exit /b 0

:bash_found
echo [all] Running build_android.sh via !BASH_EXE!
"!BASH_EXE!" "%SCRIPT_DIR%build_android.sh"
exit /b %errorlevel%

:error
echo [all] Build failed (errorlevel %errorlevel%)
exit /b %errorlevel%

