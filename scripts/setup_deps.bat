@echo off
REM Setup script for NeuroBoost dependencies on Windows
REM This script clones and builds iPlug2 and Visage

setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
set DEPS_DIR=%PROJECT_ROOT%\deps

REM Pin dependency versions to match CI (see .github/workflows/build.yml)
set IPLUG2_COMMIT=3b32d40f2b2da1f64ce2e5a6552f5bde7c76f2b1
set VISAGE_COMMIT=b0b2ee89abb3f1173dad4d80460197eb96afa724

echo Setting up NeuroBoost dependencies...
echo Project root: %PROJECT_ROOT%
echo Dependencies directory: %DEPS_DIR%
echo iPlug2 commit: %IPLUG2_COMMIT%
echo Visage commit: %VISAGE_COMMIT%

if not exist "%DEPS_DIR%" mkdir "%DEPS_DIR%"
cd /d "%DEPS_DIR%"

REM Clone and setup iPlug2
if not exist "iPlug2" (
    echo Cloning iPlug2...
    git clone https://github.com/iPlug2/iPlug2.git
    if errorlevel 1 (
        echo Failed to clone iPlug2!
        exit /b 1
    )
    cd /d iPlug2
    git checkout %IPLUG2_COMMIT%
    if errorlevel 1 (
        echo Failed to checkout iPlug2 commit!
        exit /b 1
    )
    git submodule update --init --recursive Dependencies/IPlug/VST3_SDK
    cd /d "%DEPS_DIR%"
    echo iPlug2 cloned and pinned to %IPLUG2_COMMIT%
) else (
    echo iPlug2 already exists, skipping clone
)

REM Clone and build Visage
if not exist "visage" (
    echo Cloning Visage...
    git clone https://github.com/VitalAudio/visage.git
    if errorlevel 1 (
        echo Failed to clone Visage!
        exit /b 1
    )

    cd /d visage
    git checkout %VISAGE_COMMIT%
    if errorlevel 1 (
        echo Failed to checkout Visage commit!
        exit /b 1
    )
    
    echo Building Visage...
    if not exist "build" mkdir build
    cd build
    cmake -DVISAGE_BUILD_EXAMPLES=OFF -DVISAGE_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release ..
    if errorlevel 1 (
        echo CMake configuration for Visage failed!
        exit /b 1
    )
    cmake --build . --config Release --parallel
    if errorlevel 1 (
        echo Visage build failed!
        exit /b 1
    )
    cd /d "%DEPS_DIR%"
    
    echo Visage built successfully (pinned to %VISAGE_COMMIT%)
) else (
    echo Visage already exists, checking build...
    if not exist "visage\build\Release\visage.lib" (
        if not exist "visage\build\Debug\visage.lib" (
            echo Visage not built, building now...
            cd visage
            if not exist "build" mkdir build
            cd build
            cmake -DVISAGE_BUILD_EXAMPLES=OFF -DVISAGE_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release ..
            cmake --build . --config Release --parallel
            cd /d "%DEPS_DIR%"
        )
    )
)

echo.
echo Dependencies setup complete!
echo.
echo Next steps:
echo   1. Run: scripts\build.bat
echo   2. Or use CMake directly: mkdir build ^&^& cd build ^&^& cmake .. ^&^& cmake --build . --config Release

endlocal
