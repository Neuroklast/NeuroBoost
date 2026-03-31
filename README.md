# NeuroBoost

A deterministic & probabilistic MIDI sequencer plugin built with [iPlug2](https://github.com/iPlug2/iPlug2) and [Visage](https://github.com/VitalAudio/visage) UI library.

> **Status (Sprint 2):** All 7 generation algorithms implemented and tested. Plugin fully integrated as a MIDI sequencer (MIDI out, 15 parameters, host transport sync). Sprint 3 will bring the full Visage UI redesign (step grid, knobs, visualizers).

## Documentation

- [Architecture](docs/ARCHITECTURE.md) - Technical architecture and design decisions
- [Development Workflow](docs/DEVELOPMENT_WORKFLOW.md) - Mandatory workflow for every session
- [Agent Instructions](docs/AGENTS.md) - Instructions for AI coding agents
- [Project Status](docs/STATUS.md) - Current development status and checklists
- [Lessons Learned](docs/LESSONS_LEARNED.md) - Living document of lessons learned
- [UI/UX Specification](docs/UI_UX_SPEC.md) - UI layout, interactions, and visual design
- [Market Analysis](docs/MARKET_ANALYSIS.md) - Competitive landscape and unique value

## Features

- **7 generation algorithms**: Euclidean (Bjorklund), Fibonacci, L-System, Cellular Automata (Wolfram 1D), Markov Chain, Fractal (Mandelbrot), Probability
- **15 sequencer parameters**: step count, generation mode, root note, scale, density, swing, Euclidean hits/rotation, fractal params, Markov presets, velocity, octave range
- **3 built-in Markov presets**: Blues, Jazz, Minimal (constexpr matrices, no file I/O)
- **Realtime-safe DSP**: no heap allocations, no mutex, no exceptions in audio thread
- **Deterministic output**: same seed → same sequence every run (mt19937 RNG)
- **Full MIDI output**: sample-accurate Note-On and Note-Off with iPlug2
- **Host transport sync**: PPQ-based beat tracking, loop detection
- **Scale quantization**: 15 scales with lookup tables
- Built with modern C++17
- Cross-platform support (macOS, Windows, Linux)
- Supports VST3 and standalone formats
- 103 standalone DSP tests (no iPlug2/Visage dependency)

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
- X11 development libraries (`libx11-dev`)
- OpenGL development libraries (`libgl1-mesa-dev`)
- FreeType development libraries (`libfreetype6-dev`)
- GTK3 development libraries (`libgtk-3-dev`)
- ALSA development libraries (`libasound2-dev`)

Install on Ubuntu/Debian:
```bash
sudo apt-get install libx11-dev libgl1-mesa-dev libfreetype6-dev libgtk-3-dev libasound2-dev
```

## Quick Start (DSP Test)

The simplest way to verify the project builds correctly is to compile the DSP test executable, which has no external dependencies:

```bash
mkdir build
cd build
cmake -DBUILD_DSP_TEST=ON ..
cmake --build .
ctest --output-on-failure
```

This builds and runs tests for the core audio processing functionality.

## Building

### 1. Setup Dependencies (for full plugin build)

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

**DSP Test only (no dependencies):**
```bash
mkdir build
cd build
cmake -DBUILD_DSP_TEST=ON -DBUILD_VST3=OFF -DBUILD_STANDALONE=OFF ..
cmake --build . --config Release
```

**Full plugin build:**
```bash
mkdir build
cd build
cmake -DBUILD_DSP_TEST=ON -DBUILD_VST3=ON -DBUILD_STANDALONE=ON -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_DSP_TEST` | Build DSP test executable (no dependencies) | ON |
| `BUILD_VST3` | Build VST3 plugin | OFF |
| `BUILD_STANDALONE` | Build standalone application | OFF |
| `BUILD_AU` | Build Audio Unit plugin (macOS only) | OFF |

## Output

After building, you'll find the following outputs in the `build/` directory:

- **DSP Test:** `build/bin/NeuroBoost_DSP_Test`
- **VST3 Plugin:** `build/vst3/NeuroBoost.vst3` (or `.so` on Linux)
- **Standalone App:** `build/bin/NeuroBoost` (or `NeuroBoost.exe` on Windows)

### Installing the VST3 Plugin

Copy the `.vst3` file to your VST3 plugins folder:

- **macOS:** `/Library/Audio/Plug-Ins/VST3/` or `~/Library/Audio/Plug-Ins/VST3/`
- **Windows:** `C:\Program Files\Common Files\VST3\`
- **Linux:** `~/.vst3/` or `/usr/lib/vst3/`

## Usage

### DSP Test

Run the DSP test with a gain percentage (0-200):
```bash
./build/bin/NeuroBoost_DSP_Test [gain_percent]
```

Examples:
```bash
./build/bin/NeuroBoost_DSP_Test 100  # Unity gain (no change)
./build/bin/NeuroBoost_DSP_Test 50   # Half volume
./build/bin/NeuroBoost_DSP_Test 200  # Double volume (+6dB)
./build/bin/NeuroBoost_DSP_Test 0    # Mute
```

### Plugin

1. Load the plugin in your DAW or run the standalone application
2. Use the gain knob to adjust the input gain from 0% to 200%
   - 0%: Mute the signal
   - 100%: Unity gain (no change)
   - 200%: Double the signal level (+6dB)

## Project Structure

```
NeuroBoost/
├── src/
│   ├── NeuroBoost.h      # Main plugin header (with UI)
│   ├── NeuroBoost.cpp    # Plugin implementation (with UI)
│   ├── NeuroBoostDSP.h   # DSP-only header
│   └── NeuroBoostDSP.cpp # DSP-only implementation + test
├── config.h              # Plugin configuration
├── scripts/
│   ├── setup_deps.sh     # Dependency setup (Unix)
│   ├── setup_deps.bat    # Dependency setup (Windows)
│   ├── build.sh          # Build script (Unix)
│   └── build.bat         # Build script (Windows)
├── CMakeLists.txt        # CMake build configuration
└── README.md             # This file
```

## Running Tests

```bash
cd build
ctest --output-on-failure
```

Or run individual tests:
```bash
./bin/NeuroBoost_DSP_Test 100  # Test unity gain
./bin/NeuroBoost_DSP_Test 50   # Test half gain
./bin/NeuroBoost_DSP_Test 200  # Test double gain
./bin/NeuroBoost_DSP_Test 0    # Test mute
```

## License

This project is open source. See LICENSE for details.

## Acknowledgments

- [iPlug2](https://github.com/iPlug2/iPlug2) - Cross-platform audio plugin framework
- [Visage](https://github.com/VitalAudio/visage) - Modern GPU-accelerated UI library
- Based on the [IPlugVisage example](https://github.com/iPlug2/iPlug2/pull/1192)
