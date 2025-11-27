@echo off
setlocal enableextensions enabledelayedexpansion

rem Build CYThread on Windows for multiple runtimes (MD/MT), architectures,
rem Debug/Release, and static/shared variants per requirements.

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "ROOT_DIR=%%~fI"
set "BUILD_ROOT=%ROOT_DIR%\Build\out\windows"
set "BIN_ROOT=%ROOT_DIR%\Bin\Windows"

if defined CMAKE_GENERATOR (
    set "GENERATOR=%CMAKE_GENERATOR%"
) else (
    set "GENERATOR=Visual Studio 17 2022"
)

call :ensure_vs_env
if errorlevel 1 exit /b 1

set "ARCH_LIST=x64 Win32"
set "CONFIG_LIST=Debug Release"

if not exist "%BIN_ROOT%" mkdir "%BIN_ROOT%"

call :build_loop
if errorlevel 1 exit /b 1

echo [Windows] Artifacts placed under %BIN_ROOT%
exit /b 0

:build_loop
for %%A in (%ARCH_LIST%) do (
    rem MD runtime (static + shared)
    call :configure_and_build "%%A" "MD" "ON"  "shared"
    if errorlevel 1 exit /b 1
    call :configure_and_build "%%A" "MD" "OFF" "static"
    if errorlevel 1 exit /b 1

    rem MT runtime (shared + static)
    call :configure_and_build "%%A" "MT" "ON"  "shared"
    if errorlevel 1 exit /b 1
    call :configure_and_build "%%A" "MT" "OFF" "static"
    if errorlevel 1 exit /b 1
)
exit /b 0

:configure_and_build
set "ARCH=%~1"
set "RUNTIME=%~2"
set "SHARED_FLAG=%~3"
set "LIBTYPE=%~4"
set "BUILD_DIR=%BUILD_ROOT%\%ARCH%_%RUNTIME%_%LIBTYPE%"
set "MSVC_RUNTIME="

if /i "%RUNTIME%"=="MD" (
    set "MSVC_RUNTIME=MultiThreaded$^<$^<CONFIG:Debug^>:Debug^>DLL"
) else (
    set "MSVC_RUNTIME=MultiThreaded$^<$^<CONFIG:Debug^>:Debug^>"
)

echo [Windows] Configuring %ARCH% %RUNTIME% %LIBTYPE%
cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" -A %ARCH% ^
    -DCYTHREAD_BUILD_SHARED=%SHARED_FLAG% ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=%MSVC_RUNTIME%
if errorlevel 1 exit /b 1

for %%C in (%CONFIG_LIST%) do (
    echo [Windows] Building %ARCH% %RUNTIME% %LIBTYPE% %%C
    cmake --build "%BUILD_DIR%" --config %%C
    if errorlevel 1 exit /b 1
    call :collect "%BUILD_DIR%" "%%C" "%ARCH%" "%RUNTIME%" "%LIBTYPE%"
    if errorlevel 1 exit /b 1
)
exit /b 0

:collect
set "BUILD_DIR=%~1"
set "CONFIG=%~2"
set "ARCH=%~3"
set "RUNTIME=%~4"
set "LIBTYPE=%~5"

set "SOURCE_DIR=%BUILD_DIR%\%CONFIG%"
set "TARGET_DIR=%BIN_ROOT%\%ARCH%\%RUNTIME%\%LIBTYPE%\%CONFIG%"
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"

if /i "%LIBTYPE%"=="shared" (
    copy /y "%SOURCE_DIR%\CYThread.dll" "%TARGET_DIR%\CYThread.dll" >nul
    if errorlevel 1 exit /b 1
    copy /y "%SOURCE_DIR%\CYThread.lib" "%TARGET_DIR%\CYThread.lib" >nul
    if exist "%SOURCE_DIR%\CYThread.pdb" copy /y "%SOURCE_DIR%\CYThread.pdb" "%TARGET_DIR%\CYThread.pdb" >nul
) else (
    copy /y "%SOURCE_DIR%\CYThread.lib" "%TARGET_DIR%\CYThread.lib" >nul
    if errorlevel 1 exit /b 1
    if exist "%SOURCE_DIR%\CYThread.pdb" copy /y "%SOURCE_DIR%\CYThread.pdb" "%TARGET_DIR%\CYThread.pdb" >nul
)
exit /b 0
 
:ensure_vs_env
if defined VisualStudioVersion (
    echo [Windows] Visual Studio environment already active (v%VisualStudioVersion%)
    exit /b 0
)

set "VSWHERE="
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
) else if exist "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" (
    set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)

if not defined VSWHERE (
    echo [Windows] Unable to locate vswhere.exe. Install Visual Studio 2017+.
    exit /b 1
)

for /f "usebackq tokens=* delims=" %%I in (`"%VSWHERE%" -latest -prerelease -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set "VS_INSTALL=%%I"
)

if not defined VS_INSTALL (
    echo [Windows] vswhere.exe could not find a suitable Visual Studio installation.
    exit /b 1
)

set "VS_DEV_CMD=%VS_INSTALL%\Common7\Tools\VsDevCmd.bat"
if not exist "%VS_DEV_CMD%" (
    echo [Windows] VsDevCmd.bat not found under %VS_DEV_CMD%
    exit /b 1
)

call "%VS_DEV_CMD%" -host_arch=amd64 >nul
if errorlevel 1 (
    echo [Windows] Failed to initialize Visual Studio developer command prompt.
    exit /b 1
)

echo [Windows] Loaded Visual Studio environment from %VS_INSTALL%
exit /b 0
