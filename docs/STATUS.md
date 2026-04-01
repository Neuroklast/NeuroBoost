# NeuroBoost – Project Status

> **This file must be updated at the end of every coding session.**
> Agents: read this file at the START of every session (Phase 1 of PITD workflow).

---

## Current Phase

**Sprint 2 Complete – All 7 algorithms implemented, SequencerEngine integrated into plugin**

The MIDI sequencer DSP foundation and all generation algorithms are complete.
`NeuroBoost.h/cpp` has been transformed from a gain plugin to a MIDI sequencer.
The next step is Sprint 3: full Visage UI redesign (step grid, knobs, visualizers).

---

## Version Roadmap

| Version | Name | Target | Description |
|---------|------|--------|-------------|
| **v0.1.0** | ALPHA | TBD | Euclidean mode + host sync + MIDI output + basic step grid UI |
| **v0.2.0** | ALPHA | TBD | All 7 generation modes + scale quantization |
| **v0.3.0** | BETA | TBD | Full interactive step grid + preset system + state chunks |
| **v0.4.0** | BETA | TBD | Fractal visualizer + all UI components |
| **v0.5.0** | RC | TBD | Cross-platform testing, DAW compatibility matrix |
| **v1.0.0** | RELEASE | TBD | Code signing, installer, manual |

---

## Component Status

| Component | Status | Notes |
|-----------|--------|-------|
| `src/common/Constants.h` | ✅ Done | MAX_STEPS, MAX_POLYPHONY, MAX_LSYSTEM_LENGTH, etc. |
| `src/common/Types.h` | ✅ Done | All enums and structs |
| `src/dsp/SequencerEngine` | ✅ Done | All 7 modes, multi-mode, note-offs, fractal caching |
| `src/dsp/Algorithms` | ✅ Done | Euclidean, Fibonacci, L-System, CA, Fractal, Markov, Probability |
| `src/dsp/ScaleQuantizer` | ✅ Done | 15 scales with lookup tables |
| `src/dsp/NoteTracker` | ✅ Done | Fixed-size ActiveNote array, panic |
| `src/dsp/TransportSync` | ✅ Done | PPQ tracking, beat-position math |
| `src/dsp/LockFreeQueue` | ✅ Done | SPSC ring buffer |
| `src/NeuroBoost.h/cpp` | ✅ Done | Sequencer plugin (15 params, ProcessBlock, OnParamChange) |
| `config.h` | ✅ Done | PLUG_TYPE=1, MIDI_IN/OUT=1, STATE_CHUNKS=1, 2025 copyright |
| `src/ui/EditorView` | Not Started | Sprint 3 |
| `src/ui/components/StepGrid` | Not Started | Sprint 3 |
| DSP test suite | ✅ Done | 103 tests passing (all algorithms + engine modes) |
| MIDI output | ✅ Done | Note-On + Note-Off via SendMidiMsg |
| Host sync (PPQ) | ✅ Done | TransportSync |
| Preset system | Not Started | Sprint 3 |
| State chunks (serialization) | Not Started | Sprint 3 (config ready) |
| Scale quantization | ✅ Done | 15 scales |
| Euclidean generation mode | ✅ Done | |
| Fibonacci generation mode | ✅ Done | |
| L-System generation mode | ✅ Done | Double-buffer, no heap |
| Cellular Automata mode | ✅ Done | Wolfram 1D, 0-255 rules |
| Markov Chain mode | ✅ Done | 3 built-in presets (Blues, Jazz, Minimal) |
| Fractal Mapping mode | ✅ Done | Mandelbrot, cacheable |
| Probability mode | ✅ Done | Per-step probability, deterministic RNG |
| CI – DSP test | ✅ Working | GitHub Actions `build.yml` |
| CI – Full plugin build | ⚠️ Partial | `continue-on-error: true` (deps not cached) |
| Documentation | ✅ Done | Sprint 2 complete |

---

## Current Sprint / Focus

### Sprint 0 – Architecture & Documentation ✅

- [x] docs/ARCHITECTURE.md created
- [x] docs/AGENTS.md created
- [x] docs/STATUS.md created
- [x] docs/LESSONS_LEARNED.md created
- [x] docs/DEVELOPMENT_WORKFLOW.md created
- [x] docs/UI_UX_SPEC.md created
- [x] docs/MARKET_ANALYSIS.md created
- [x] README.md updated with Documentation section

### Sprint 1 – DSP Foundation ✅

- [x] Create `src/common/` (Constants.h, Types.h)
- [x] Implement `LockFreeQueue.h` (SPSC ring buffer)
- [x] Implement `NoteTracker.h`
- [x] Implement `TransportSync.h`
- [x] Implement `SequencerEngine` with Euclidean mode
- [x] Add DSP tests for all Sprint 1 components

### Sprint 2 – Remaining Algorithms, Plugin Integration, config.h Migration ✅

- [x] A1: Fibonacci rhythm generator
- [x] A2: L-System pattern generator (char[MAX_LSYSTEM_LENGTH], double-buffer)
- [x] A3: Cellular Automata (Wolfram 1D, rule 0-255)
- [x] A4: Fractal mapping (Mandelbrot, cacheable)
- [x] A5: Markov chain + 3 constexpr presets (Blues, Jazz, Minimal)
- [x] A6: Probability / weighted random
- [x] B1: config.h updated (PLUG_TYPE=1, MIDI_IN/OUT=1, STATE_CHUNKS=1, 2025 copyright)
- [x] B2: NeuroBoost.h – new EParams (15 params), SequencerEngine member, LockFreeQueue
- [x] B3: NeuroBoost.cpp – ProcessBlock (MIDI out), OnParamChange, OnIdle, OnReset
- [x] B4: SequencerEngine – setGenerationMode, regeneratePattern, all modes
- [x] B5: Note-Off output (getNoteOffNotes, getNoteOffCount)
- [x] C1: Algorithm tests (Fibonacci, L-System, CA, Fractal, Markov, Probability)
- [x] C2: Engine tests (multi-mode, note-offs, stepCount=1, regeneratePattern)
- [x] D1-D3: Docs updated

### Sprint 3 – Full Visage UI Redesign ✅

- [x] Step grid component (64 cells, playhead, click to toggle)
- [x] Mode selector dropdown
- [x] Scale/root note selector
- [x] Per-mode parameter panel (Euclidean hits/rotation, Fractal params, etc.)
- [x] Playhead animation via LockFreeQueue
- [x] Transport bar (play/stop/reset)

### Sprint 4 – SequencerEngine Hardening ✅

- [x] Ratchets, swing, condition modes, mutation, velocity curve, octave range
- [x] Markov/L-System/CA/Fractal parameter accessors
- [x] State serialization (SerializeState / UnserializeState)
- [x] 10 factory presets via Presets.h
- [x] 133 DSP tests passing

### Sprint 5 – Alpha-Quality Polish + Visual UI Tests ✅

- [x] Parameter smoothing (ParamSmoother 1-pole IIR, Goal 1)
- [x] Fix OnIdle() host sync — knobs update from automation (Goal 2)
- [x] Full StepGrid interaction: shift+drag probability, right-click accent (Goal 3)
- [x] Knob↔host normalization + Begin/EndInformHostOfParamChange (Goal 4)
- [x] Hardened Note-Off: sample-accurate offsets, new timing tests (Goal 5)
- [x] Panic on transport stop/bypass/close — sendPanicNoteOffs() (Goal 6)
- [x] UI structure test (test_ui_structure.cpp + ASCII dump) (Goal 7)
- [x] 156 DSP tests + 38 UI structure tests passing

---

## Milestone Checklists

### ALPHA Milestone

- [ ] Euclidean rhythm generation works correctly
- [ ] MIDI output is sample-accurate (correct sampleOffset for every event)
- [ ] Note-Offs are always sent (no hung notes in any DAW)
- [ ] UI shows animated playhead on step grid
- [ ] Determinism verified: same seed → same sequence, 100% of the time
- [ ] DSP tests pass without UI dependencies
- [ ] Plugin loads in Ableton Live 11+ without crash
- [ ] Transport start/stop/loop cycle without hung notes

### BETA Milestone

- [ ] All 7 generation modes implemented (Euclidean, L-System, CA, Markov, Fractal, Random Walk, User)
- [ ] 15+ scales implemented with correct quantization
- [ ] Interactive step grid (click to toggle, drag velocity, right-click menu)
- [ ] Preset system: load/save/factory presets
- [ ] State chunks: full serialization with version migration
- [ ] No crash on rapid tempo change, loop boundary, bypass toggle
- [ ] Tested on macOS 12+ and Windows 10+
- [ ] Tested in: Ableton Live, Logic Pro, Reaper, FL Studio, Bitwig Studio

### RELEASE Milestone

- [ ] Code signing (macOS notarization, Windows Authenticode)
- [ ] Installer (macOS `.pkg`, Windows `.msi`)
- [ ] LICENSE file present and correct
- [ ] User manual (PDF or HTML)
- [ ] No known P1/P2 bugs
- [ ] CPU usage < 1% on reference machine (MacBook Pro M1, 256-sample buffer)
- [ ] Accessibility: keyboard navigation for all controls
- [ ] Linux tested (Ubuntu 22.04 LTS, VST3)

### Gold Standard Milestone

- [ ] MIDI Learn for all parameters
- [ ] MPE output (per-note pitch bend, pressure, slide)
- [ ] Pattern import/export (MIDI file, JSON)
- [ ] Responsive UI (scales from 400×300 to 1600×1200)
- [ ] HiDPI / Retina support
- [ ] Tooltips on all controls
- [ ] Crash reporting (opt-in)
- [ ] CLAP format support
- [ ] AU format support (macOS)

---

## Change Log

> Agents: append an entry here after every session using the format below.

```
### [DATE] – [Short description]
- What changed: ...
- Files modified: ...
- Tests updated: ...
- Known issues introduced: ...
```

### [2026-03-31] – Sprint 2: Algorithms, Plugin Integration, config.h Migration
- What changed: Added 6 new generation algorithms; integrated SequencerEngine
  into NeuroBoost plugin; updated config.h for MIDI sequencer; 103 tests pass.
- Files modified: src/dsp/Algorithms.h, src/dsp/Algorithms.cpp,
  src/dsp/SequencerEngine.h, src/dsp/SequencerEngine.cpp,
  src/NeuroBoost.h, src/NeuroBoost.cpp, config.h,
  tests/test_sequencer.cpp, docs/STATUS.md, docs/LESSONS_LEARNED.md, README.md
- Tests updated: 103 tests passing (up from 30); new sections: Fibonacci,
  LSystem, CellularAutomata, Fractal, Markov, Probability, multi-mode engine
- Known issues introduced: see below

### [2025-03-31] – Sprint 0: Architecture & Documentation
- What changed: Created complete documentation suite for MIDI sequencer transition
- Files modified: docs/ARCHITECTURE.md, docs/AGENTS.md, docs/STATUS.md,
  docs/LESSONS_LEARNED.md, docs/DEVELOPMENT_WORKFLOW.md,
  docs/UI_UX_SPEC.md, docs/MARKET_ANALYSIS.md, README.md
- Tests updated: none (documentation only)
- Known issues introduced: none

---

## Known Issues

> Agents: add new issues here; mark resolved issues with ✅.

### From existing gain plugin code

| # | Severity | Issue | File | Status |
|---|----------|-------|------|--------|
| 1 | 🔴 High | `SendParameterValueFromUI` passes raw gain value instead of normalized (0–1) | `src/NeuroBoost.cpp` | ✅ Fixed (Sprint 2) |
| 2 | 🔴 High | No parameter smoothing in ProcessBlock → zipper noise on automation | `src/NeuroBoost.cpp` | ✅ Fixed (Sprint 5, ParamSmoother) |
| 3 | 🟡 Med | `OnIdle()` is empty → host automation changes don't sync to UI knob | `src/NeuroBoost.cpp` | ✅ Fixed (Sprint 5, updateKnobFromHost) |
| 4 | 🟡 Med | `EParams` enum defined in both `NeuroBoost.h` and `NeuroBoostDSP.h` → ODR violation risk | both | Open (NeuroBoostDSP.h kept for legacy DSP tests) |
| 5 | 🟢 Low | `config.h` copyright says 2024, should be 2025 | `config.h:9` | ✅ Fixed (Sprint 2) |
| 6 | 🟢 Low | README references LICENSE file which does not exist | `README.md` | Open |
| 7 | 🟡 Med | `std::ostringstream` used in draw callback (heap allocation in UI frame) | `src/NeuroBoost.cpp` | Open (Sprint 3 UI redesign) |
| 8 | 🟡 Med | `visage::Font` created every frame in draw callback | `src/NeuroBoost.cpp` | Open (Sprint 3 UI redesign) |
| 9 | 🟢 Low | CI full-plugin build has `continue-on-error: true` → silent failures | `.github/workflows/build.yml` | Open |
| 10 | 🟢 Low | Dependency pinned to `GIT_TAG master` → not reproducible | `CMakeLists.txt` | Open |
