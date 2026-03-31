# NeuroBoost – Project Status

> **This file must be updated at the end of every coding session.**
> Agents: read this file at the START of every session (Phase 1 of PITD workflow).

---

## Current Phase

**Pre-Development / Architecture & Documentation**

The existing codebase is a gain plugin placeholder. The MIDI sequencer core
has not yet been implemented. All architecture documentation has been created
in this sprint. The next step is Sprint 1: implement the DSP foundation.

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
| `src/common/Constants.h` | Not Started | MAX_STEPS, MAX_POLYPHONY, etc. |
| `src/common/Types.h` | Not Started | SequencerState, StepData, structs |
| `src/common/Params.h` | Not Started | Single EParams definition |
| `src/dsp/SequencerEngine` | Not Started | Main audio-thread loop |
| `src/dsp/Algorithms` | Not Started | Euclidean, L-System, CA, Markov, Fractal |
| `src/dsp/ScaleQuantizer` | Not Started | 15+ scales, lookup table |
| `src/dsp/NoteTracker` | Not Started | ActiveNote array, panicAllNotesOff |
| `src/dsp/TransportSync` | Not Started | PPQ tracking, beat-position math |
| `src/dsp/LockFreeQueue` | Not Started | SPSC ring buffer |
| `src/ui/EditorView` | Not Started | Top-level Visage Frame |
| `src/ui/components/StepGrid` | Not Started | 64-step grid + playhead |
| `src/ui/components/StepCell` | Not Started | Individual step cell |
| `src/ui/components/ProbabilityBar` | Not Started | Per-step probability widget |
| `src/ui/components/Knob` | Not Started | Reusable knob (replaces GainKnob) |
| `src/ui/components/Selector` | Not Started | Mode / scale dropdown |
| `src/ui/components/PianoRoll` | Not Started | Pitch editing |
| `src/ui/components/FractalView` | Not Started | GPU fractal visualizer |
| `src/ui/theme/Theme.h` | Not Started | Colors, fonts, spacing |
| DSP test suite | Partial | Gain tests pass; sequencer tests needed |
| MIDI output | Not Started | |
| Host sync (PPQ) | Not Started | |
| Preset system | Not Started | |
| State chunks (serialization) | Not Started | |
| Scale quantization | Not Started | |
| Euclidean generation mode | Not Started | |
| L-System generation mode | Not Started | |
| Cellular Automata mode | Not Started | |
| Markov Chain mode | Not Started | |
| Fractal Mapping mode | Not Started | |
| Random Walk mode | Not Started | |
| User Pattern mode | Not Started | |
| CI – DSP test | ✅ Working | GitHub Actions `build.yml` |
| CI – Full plugin build | ⚠️ Partial | `continue-on-error: true` (deps not cached) |
| Documentation | ✅ Done | Sprint 0 complete |

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

### Sprint 1 – DSP Foundation (Next)

- [ ] Create `src/common/` (Constants.h, Types.h, Params.h)
- [ ] Implement `LockFreeQueue.h` (SPSC ring buffer)
- [ ] Implement `NoteTracker.h`
- [ ] Implement `TransportSync.h`
- [ ] Implement `SequencerEngine` skeleton (ProcessBlock, transport detection)
- [ ] Add DSP tests for NoteTracker and TransportSync
- [ ] Fix all Known Issues from the gain plugin (see below)

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
| 1 | 🔴 High | `SendParameterValueFromUI` passes raw gain value instead of normalized (0–1) | `src/NeuroBoost.cpp:163` | Open |
| 2 | 🔴 High | No parameter smoothing in ProcessBlock → zipper noise on automation | `src/NeuroBoost.cpp:208` | Open |
| 3 | 🟡 Med | `OnIdle()` is empty → host automation changes don't sync to UI knob | `src/NeuroBoost.cpp:114` | Open |
| 4 | 🟡 Med | `EParams` enum defined in both `NeuroBoost.h` and `NeuroBoostDSP.h` → ODR violation risk | both | Open |
| 5 | 🟢 Low | `config.h` copyright says 2024, should be 2025 | `config.h:9` | Open |
| 6 | 🟢 Low | README references LICENSE file which does not exist | `README.md` | Open |
| 7 | 🟡 Med | `std::ostringstream` used in draw callback (heap allocation in UI frame) | `src/NeuroBoost.cpp` | Open |
| 8 | 🟡 Med | `visage::Font` created every frame in draw callback | `src/NeuroBoost.cpp` | Open |
| 9 | 🟢 Low | CI full-plugin build has `continue-on-error: true` → silent failures | `.github/workflows/build.yml` | Open |
| 10 | 🟢 Low | Dependency pinned to `GIT_TAG master` → not reproducible | `CMakeLists.txt` | Open |
