# NeuroBoost

A deterministic & probabilistic MIDI sequencer plugin built with [iPlug2](https://github.com/iPlug2/iPlug2) and [Visage](https://github.com/VitalAudio/visage) UI library.

> **Status (Sprint 7):** Full Visage UI, 7 generation algorithms, 16 parameters, MIDI input modes, preset browser, keyboard shortcuts, MIDI export. 241 DSP tests + 38 UI structure tests passing on all platforms.

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
- **16 sequencer parameters**: step count, generation mode, root note, scale, density, swing, Euclidean hits/rotation, fractal seed/dimension/depth, mutation rate, velocity curve, octave range, MIDI input mode
- **6 MIDI input modes**: Off, Thru, Transpose, Trigger, Gate, Thru+Generate
- **10 factory presets**: Classic 4/4, Euclidean 7/16, Golden Ratio, Fractal Forest, Rule 110, Mandelbrot Dance, Blues Walker, Jazz Stroll, Sparse Probability, Euclidean Shuffle
- **3 built-in Markov presets**: Blues, Jazz, Minimal (constexpr matrices, no file I/O)
- **Realtime-safe DSP**: no heap allocations, no mutex, no exceptions in audio thread
- **Deterministic output**: same seed → same sequence every run (mt19937 RNG)
- **Full MIDI output**: sample-accurate Note-On and Note-Off with iPlug2
- **Host transport sync**: PPQ-based beat tracking, loop detection
- **Scale quantization**: 15 scales with lookup tables
- **MIDI export**: Standard MIDI File (SMF Format 0) writer
- **Preset browser**: Browse/recall factory presets with dirty state indicator
- **Keyboard shortcuts**: Number keys for mode select, arrow keys for step navigation, Space for transport
- **Visage GPU-accelerated UI**: 900×600 resizable frame with step grid, knobs, selectors
- Built with modern C++17
- Cross-platform support (macOS, Windows, Linux)
- Supports VST3 and standalone formats
- 241 standalone DSP tests + 38 UI structure tests (no iPlug2/Visage dependency)

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

- **DSP Test:** `build/bin/test_sequencer`
- **UI Structure Test:** `build/bin/test_ui_structure`
- **VST3 Plugin:** `build/vst3/NeuroBoost.vst3` (bundle on macOS/Linux, DLL on Windows)
- **Standalone App:** `build/bin/NeuroBoost` (or `NeuroBoost.exe` on Windows)

### Installing the VST3 Plugin

Copy the `.vst3` file to your VST3 plugins folder:

- **macOS:** `/Library/Audio/Plug-Ins/VST3/` or `~/Library/Audio/Plug-Ins/VST3/`
- **Windows:** `C:\Program Files\Common Files\VST3\`
- **Linux:** `~/.vst3/` or `/usr/lib/vst3/`

## Usage

### Running Tests

```bash
cd build
ctest --output-on-failure
```

The test suite includes 241 DSP/sequencer tests and 38 UI structure tests that verify algorithms, transport sync, MIDI export, and UI layout correctness — all without requiring iPlug2 or Visage.

### Plugin

1. Load the plugin in your DAW or run the standalone application
2. Select a generation algorithm using the mode selector or number keys 1–7
3. Configure scale, root note, and rhythm parameters via knobs
4. Use the step grid to toggle individual steps, adjust velocity (drag), and accent (right-click)
5. Browse factory presets with the preset browser bar
6. Export patterns as MIDI files with the E key

## Project Structure

```
NeuroBoost/
├── src/
│   ├── NeuroBoost.h/.cpp    # Main plugin (iPlug2 Plugin subclass, UI wiring)
│   ├── dsp/                  # Realtime-safe DSP engine
│   │   ├── SequencerEngine.h/.cpp  # Core MIDI sequencer
│   │   ├── Algorithms.h/.cpp       # 7 generation algorithms
│   │   ├── ScaleQuantizer.h        # 15-scale pitch quantization
│   │   ├── NoteTracker.h           # Polyphonic note-off tracking
│   │   ├── TransportSync.h         # Host PPQ transport sync
│   │   ├── ParamSmoother.h         # 1-pole IIR parameter smoother
│   │   └── LockFreeQueue.h         # SPSC lock-free ring buffer
│   ├── ui/                   # Visage GPU-accelerated UI
│   │   ├── EditorView.h/.cpp       # Main 900×600 editor frame
│   │   ├── components/             # StepGrid, ParameterKnob, ModeSelector, TransportBar, PresetBrowser, StepCell
│   │   └── theme/Theme.h           # Centralized color definitions
│   └── common/               # Shared types and utilities
│       ├── Types.h                 # Enums (ScaleMode, GenerationMode, MidiInputMode, etc.)
│       ├── Constants.h             # MAX_STEPS, MAX_POLYPHONY, etc.
│       ├── Presets.h               # 10 factory presets (constexpr)
│       └── MidiExport.h/.cpp       # SMF Format 0 MIDI file writer
├── tests/
│   ├── test_sequencer.cpp    # 241 DSP tests (no external deps)
│   └── test_ui_structure.cpp # 38 UI structure tests (mock-based)
├── config.h                  # Plugin configuration (iPlug2)
├── resource.h                # Windows resource definitions
├── scripts/
│   ├── setup_deps.sh/.bat    # Dependency setup (pinned commits)
│   └── build.sh/.bat         # Build scripts
├── CMakeLists.txt            # CMake build configuration
└── README.md                 # This file
```

## Running Tests

```bash
cd build
ctest --output-on-failure
```

Or run individual test binaries:
```bash
./build/bin/test_sequencer       # 241 DSP/sequencer tests
./build/bin/test_ui_structure    # 38 UI structure tests
```

## License

This software is proprietary. See LICENSE for details.

## Acknowledgments

- [iPlug2](https://github.com/iPlug2/iPlug2) - Cross-platform audio plugin framework
- [Visage](https://github.com/VitalAudio/visage) - Modern GPU-accelerated UI library
- Based on the [IPlugVisage example](https://github.com/iPlug2/iPlug2/pull/1192)
