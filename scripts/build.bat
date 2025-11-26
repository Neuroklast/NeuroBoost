@echo off
REM Build script for NeuroBoost on Windows
REM Builds VST3 plugin and standalone application

setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
set BUILD_DIR=%PROJECT_ROOT%\build

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

echo Building NeuroBoost...
echo   Build type: %BUILD_TYPE%
echo   Build directory: %BUILD_DIR%

REM Check if dependencies are set up
if not exist "%PROJECT_ROOT%\deps\iPlug2" (
    echo.
    echo Dependencies not found. Please run setup_deps.bat first.
    exit /b 1
)

if not exist "%PROJECT_ROOT%\deps\visage" (
    echo.
    echo Dependencies not found. Please run setup_deps.bat first.
    exit /b 1
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Configure with CMake
echo.
echo Configuring with CMake...
cmake ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_VST3=ON ^
    -DBUILD_STANDALONE=ON ^
    ..

if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build
echo.
echo Building...
cmake --build . --config %BUILD_TYPE%

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build complete!
echo.
echo Output locations:
echo   VST3: %BUILD_DIR%\%BUILD_TYPE%\NeuroBoost.vst3
echo   Standalone: %BUILD_DIR%\%BUILD_TYPE%\NeuroBoost.exe

endlocal
