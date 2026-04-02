@echo off
REM Build script for NeuroBoost on Windows
REM Builds DSP test, VST3 plugin, and standalone application

setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
set BUILD_DIR=%PROJECT_ROOT%\build

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

echo Building NeuroBoost...
echo   Build type: %BUILD_TYPE%
echo   Build directory: %BUILD_DIR%

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Check if we can build full plugin
set BUILD_FULL_PLUGIN=OFF
if exist "%PROJECT_ROOT%\deps\iPlug2" (
    if exist "%PROJECT_ROOT%\deps\visage" (
        if exist "%PROJECT_ROOT%\deps\visage\build\Release\visage.lib" (
            set BUILD_FULL_PLUGIN=ON
            echo   Full plugin build: enabled
        ) else if exist "%PROJECT_ROOT%\deps\visage\build\Debug\visage.lib" (
            set BUILD_FULL_PLUGIN=ON
            echo   Full plugin build: enabled
        ) else (
            echo   Full plugin build: disabled (Visage not built)
        )
    ) else (
        echo   Full plugin build: disabled (Visage not found)
    )
) else (
    echo   Full plugin build: disabled (iPlug2 not found)
)

REM Configure with CMake
echo.
echo Configuring with CMake...
cmake ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_DSP_TEST=ON ^
    -DBUILD_VST3=%BUILD_FULL_PLUGIN% ^
    -DBUILD_STANDALONE=%BUILD_FULL_PLUGIN% ^
    ..

if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build
echo.
echo Building...
cmake --build . --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

REM Run tests
echo.
echo Running tests...
ctest --output-on-failure

echo.
echo Build complete!
echo.
echo Output locations:
echo   DSP Test: %BUILD_DIR%\bin\test_sequencer.exe
if "%BUILD_FULL_PLUGIN%"=="ON" (
    echo   Standalone: %BUILD_DIR%\bin\
    echo   VST3: %BUILD_DIR%\vst3\
)

endlocal
