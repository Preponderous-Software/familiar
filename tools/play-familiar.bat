@echo off
setlocal
rem Plays Familiar (a Halloween night on the Strata engine) from the latest
rem main branch, building it first with the Visual Studio C++ build tools.
rem Controls: mouse look, W A S D walk, Shift hurry, E knock, T tell a joke,
rem 1-4 costume, Q cat or Wren, F1 help, Esc frees the mouse.
rem Extra arguments are passed to the game, e.g.:
rem   play-familiar.bat --seed 7     another night in the same town
rem   play-familiar.bat --tour       watch the fixed walk through town into the night
rem Set FAMILIAR_PLAY_DIR to clone and build somewhere other than
rem %USERPROFILE%\familiar-play.

if not defined FAMILIAR_PLAY_DIR set "FAMILIAR_PLAY_DIR=%USERPROFILE%\familiar-play"
set "FM_DIR=%FAMILIAR_PLAY_DIR%"
set "FM_REPO=https://github.com/Preponderous-Software/familiar.git"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "INSTALLED="

where git >nul 2>nul || (echo Git is not installed: https://git-scm.com/download/win & pause & exit /b 1)

where cmake >nul 2>nul || (
    echo Familiar needs CMake 3.24 or newer. Installing it with winget ...
    winget install --id Kitware.CMake -e --accept-source-agreements --accept-package-agreements || (echo Install it from https://cmake.org/download/ & pause & exit /b 1)
    set "INSTALLED=1"
)

rem The newest Visual Studio that has the C++ tools. That may be an older
rem one: Visual Studio 2019 with C++ beside a 2022 without it builds with 2019.
set "VS_YEAR="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productLineVersion`) do set "VS_YEAR=%%i"
)
if not defined VS_YEAR (
    echo Familiar needs the Visual Studio C++ build tools. Installing them with winget, this takes a while ...
    winget install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-source-agreements --accept-package-agreements --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" || (echo Install "Desktop development with C++" from https://visualstudio.microsoft.com/visual-cpp-build-tools/ & pause & exit /b 1)
    set "INSTALLED=1"
)

if defined INSTALLED (
    echo Tools installed. Close this window and run the script again.
    pause
    exit /b 0
)

set "FM_GEN="
if "%VS_YEAR%"=="2019" set "FM_GEN=Visual Studio 16 2019"
if "%VS_YEAR%"=="2022" set "FM_GEN=Visual Studio 17 2022"

if not exist "%FM_DIR%\.git" (
    echo Cloning Familiar into %FM_DIR% ...
    git clone -b main "%FM_REPO%" "%FM_DIR%" || (pause & exit /b 1)
) else (
    echo Updating %FM_DIR% ...
    pushd "%FM_DIR%"
    git fetch origin || (popd & pause & exit /b 1)
    git checkout -B main origin/main || (popd & pause & exit /b 1)
    popd
)

pushd "%FM_DIR%"
echo Building with Visual Studio %VS_YEAR% (the first time fetches and builds Strata and SDL3, a few minutes) ...
rem The generator is chosen once; a build directory keeps the one it was made with.
if exist "build\CMakeCache.txt" (
    cmake -S . -B build -DFAMILIAR_BUILD_TESTS=OFF -Wno-dev || (popd & pause & exit /b 1)
) else if defined FM_GEN (
    cmake -S . -B build -G "%FM_GEN%" -A x64 -DFAMILIAR_BUILD_TESTS=OFF -Wno-dev || (popd & pause & exit /b 1)
) else (
    cmake -S . -B build -A x64 -DFAMILIAR_BUILD_TESTS=OFF -Wno-dev || (popd & pause & exit /b 1)
)
cmake --build build --config Release --parallel --target familiar || (popd & pause & exit /b 1)

build\Release\familiar.exe %*
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" pause
exit /b %RC%
