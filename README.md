# NeuroBoost

A simple audio gain plugin built with [iPlug2](https://github.com/iPlug2/iPlug2) and [Visage](https://github.com/VitalAudio/visage) UI library.

![NeuroBoost Plugin](docs/screenshot.png)

## Features

- Simple gain control with a single knob
- Built with modern C++17
- Cross-platform support (macOS, Windows, Linux)
- Supports VST3 and standalone formats

## Requirements

- CMake 3.16 or higher
- C++17 compatible compiler
- Git (for cloning dependencies)

### Platform-specific requirements:

**macOS:**
- Xcode Command Line Tools
- macOS 10.15 or later

**Windows:**
- Visual Studio 2019 or later with C++ support
- Windows SDK

**Linux:**
- GCC 9+ or Clang 10+
- X11 development libraries
- ALSA development libraries

## Building

### 1. Setup Dependencies

First, download and build the required dependencies (iPlug2 and Visage):

**macOS/Linux:**
```bash
./scripts/setup_deps.sh
```

**Windows:**
```batch
scripts\setup_deps.bat
```

This will clone iPlug2 and Visage into the `deps/` directory and build Visage.

### 2. Build the Plugin

**macOS/Linux:**
```bash
./scripts/build.sh
```

**Windows:**
```batch
scripts\build.bat
```

Or using CMake directly:
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

### Build Options

- `BUILD_VST3=ON/OFF` - Build VST3 plugin (default: ON)
- `BUILD_STANDALONE=ON/OFF` - Build standalone application (default: ON)
- `BUILD_AU=ON/OFF` - Build Audio Unit plugin, macOS only (default: OFF)

## Output

After building, you'll find the following outputs in the `build/` directory:

- **VST3 Plugin:** `NeuroBoost.vst3`
- **Standalone App:** `NeuroBoost` (or `NeuroBoost.exe` on Windows)

### Installing the VST3 Plugin

Copy the `.vst3` file to your VST3 plugins folder:

- **macOS:** `/Library/Audio/Plug-Ins/VST3/` or `~/Library/Audio/Plug-Ins/VST3/`
- **Windows:** `C:\Program Files\Common Files\VST3\`
- **Linux:** `~/.vst3/` or `/usr/lib/vst3/`

## Usage

1. Load the plugin in your DAW or run the standalone application
2. Use the gain knob to adjust the input gain from 0% to 200%
   - 0%: Mute the signal
   - 100%: Unity gain (no change)
   - 200%: Double the signal level (+6dB)

## Project Structure

```
NeuroBoost/
├── src/
│   ├── NeuroBoost.h      # Main plugin header
│   └── NeuroBoost.cpp    # Plugin implementation
├── config.h              # Plugin configuration
├── scripts/
│   ├── setup_deps.sh     # Dependency setup (Unix)
│   ├── setup_deps.bat    # Dependency setup (Windows)
│   ├── build.sh          # Build script (Unix)
│   └── build.bat         # Build script (Windows)
├── CMakeLists.txt        # CMake build configuration
└── README.md             # This file
```

## License

This project is open source. See LICENSE for details.

## Acknowledgments

- [iPlug2](https://github.com/iPlug2/iPlug2) - Cross-platform audio plugin framework
- [Visage](https://github.com/VitalAudio/visage) - Modern GPU-accelerated UI library
- Based on the [IPlugVisage example](https://github.com/iPlug2/iPlug2/pull/1192)
