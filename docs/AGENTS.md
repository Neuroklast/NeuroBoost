# NeuroBoost – Agent Instructions

> **Read this document before writing any code.**
> These rules exist because past sessions produced subtle bugs that broke
> realtime safety, determinism, or host compatibility. Each rule maps to
> a concrete past failure or known pitfall.

---

## Project Context

| Key | Value |
|---|---|
| **Plugin type** | MIDI Sequencer (VST3 / Standalone) |
| **Core framework** | iPlug2 (audio plugin host bridge) |
| **UI library** | Visage (GPU-accelerated, Metal/Vulkan/D3D11/OpenGL) |
| **Build system** | CMake 3.16+, C++17 |
| **Primary output** | MIDI (no audio processing) |
| **Target DAWs** | Ableton Live, Logic Pro, Reaper, FL Studio, Bitwig Studio |

For full technical details, see [ARCHITECTURE.md](ARCHITECTURE.md).

---

## Workflow Protocol (MANDATORY)

Every agent session **must** follow the PITD cycle:

```
PHASE 1: PLAN (before writing any code)
  1. Read docs/STATUS.md       → understand current state
  2. Read docs/LESSONS_LEARNED.md → avoid past mistakes
  3. Identify the task scope
  4. Write your plan as a comment/summary BEFORE coding
  5. Identify risks and edge cases upfront

PHASE 2: IMPLEMENT
  1. Follow the plan from Phase 1
  2. If something doesn't work: STOP, go back to Phase 1, revise the plan
  3. Do NOT push through a failing approach
  4. Every change must pass the realtime-safety checklist (see below)

PHASE 3: TEST
  1. DSP tests must compile and pass WITHOUT UI dependencies
  2. Verify determinism: same seed → same output, every run
  3. Test edge cases: stepCount=1, tempo=20, nFrames=1, prob=0, prob=1
  4. If tests fail: go back to Phase 1, do NOT just patch the test

PHASE 4: DOCUMENT
  1. Update docs/STATUS.md (component table + change log + known issues)
  2. Add new lessons to docs/LESSONS_LEARNED.md
  3. Update checklists in STATUS.md
  4. Commit with a descriptive message that references the plan
```

**If you skip Phase 1, you will introduce bugs. If you skip Phase 4, the next
agent will repeat your mistakes.**

---

## The 10 Critical Rules

### Rule 1 — Realtime Safety

**Audio thread is a hard-realtime context. One wrong call can cause an
audio dropout or crash.**

**FORBIDDEN in audio thread:**

```
new / delete / malloc / free       → heap allocation, can block
std::vector::push_back             → may reallocate
std::string construction           → heap allocation
std::mutex::lock                   → can block indefinitely
std::cout / printf / any I/O       → syscall, blocking
std::map / unordered_map inserts   → heap allocation
std::filesystem::*                 → kernel call
throw / exceptions                 → can allocate
std::function                      → closure may heap-allocate
std::random_device                 → reads from OS entropy pool
time() / clock()                   → OS call
```

**ALLOWED in audio thread:**

```
Fixed-size arrays (stack or member)
std::atomic<T> (lock-free specializations only)
Lock-free SPSC ring buffers (pre-allocated)
std::mt19937 (deterministic, no heap, no OS)
memcpy / memset
std::clamp / std::abs / <cmath>
GetParam(k)->Value()
SendMidiMsg()
```

### Rule 2 — Determinism

```cpp
// CORRECT:
std::mt19937 mRng;                    // member, pre-allocated
mRng.seed(userSeed);                  // on transport start only

// FORBIDDEN:
std::random_device rd; mRng.seed(rd()); // non-deterministic
mRng.seed(time(nullptr));               // non-deterministic
rand()                                  // global state, no seed control
```

Same seed → same sequence → same musical output, every run, every DAW.
Re-seed only on transport start (play-state transition false → true).

### Rule 3 — Sample-Accurate Timing

```cpp
// CORRECT: compute exact sample offset for each MIDI event
int sampleOffset = static_cast<int>(
    (stepBeatPosition - blockStartBeat) / beatsPerSample
);
sampleOffset = std::clamp(sampleOffset, 0, nFrames - 1);
noteOn.MakeNoteOnMsg(pitch, velocity, sampleOffset);

// FORBIDDEN:
noteOn.MakeNoteOnMsg(pitch, velocity, 0);  // always offset 0 = inaccurate
```

See [ARCHITECTURE.md – Timing Architecture](ARCHITECTURE.md#timing-architecture).

### Rule 4 — Note-Off Tracking

Every Note-On **must** be followed by a Note-Off. Leaking notes cause hung
MIDI in the DAW (notes play forever even after the plugin is removed).

```cpp
// On every Note-On: register in NoteTracker
mNoteTracker.registerNote(pitch, channel, noteOffBeat);

// Every ProcessBlock: check for due Note-Offs
mNoteTracker.processBlock(blockStart, blockEnd, beatsPerSample, nFrames);

// On transport stop / bypass / OnReset / CloseWindow: panic
mNoteTracker.panicAllNotesOff(0);
```

### Rule 5 — Parameter Handling

```cpp
// CORRECT: SendParameterValueFromUI takes NORMALIZED (0.0–1.0) value
double normalized = rawValue / kParamMaxValue;
BeginInformHostOfParamChange(kMyParam);
SendParameterValueFromUI(kMyParam, normalized);   // ← normalized!
EndInformHostOfParamChange(kMyParam);

// FORBIDDEN:
SendParameterValueFromUI(kMyParam, rawValue);     // raw value → broken DAW automation
```

Also: always call `Begin.../End...` around parameter changes from UI.
Omitting these breaks DAW undo and automation recording.

### Rule 6 — Visage UI Patterns

```cpp
// Frame: inherit visage::Frame, override draw(Canvas&)
class MyComponent : public visage::Frame {
    visage::Font mFont;       // ← member, created once in constructor
    bool         mDirty = true;

    void draw(visage::Canvas& canvas) override {
        // FORBIDDEN inside draw():
        //   visage::Font font("Roboto", 16);  ← creates font every frame!
        //   std::string s = "hello";           ← heap allocation
        //   new MyThing();                     ← heap allocation

        canvas.setFillColor(mColor);
        canvas.roundedRectangle(bounds(), 4.0f);
    }

    void triggerRedraw() { mDirty = true; redraw(); }
};

// Callbacks: use visage::CallbackList, NOT std::function
visage::CallbackList<void(int)> onStepClicked;
// NOT: std::function<void(int)> onStepClicked;  ← heap-allocates
```

### Rule 7 — Thread Communication

```cpp
// UI → Audio:   iPlug2 parameter system (SendParameterValueFromUI)
// Audio → UI:   Lock-free SPSC ring buffer (push in ProcessBlock, pop in OnIdle)
// Complex data: Double-buffer + atomic swap (fractal results, pattern snapshots)

// FORBIDDEN:
// std::mutex protected data read on both threads (may block audio thread)
// Raw pointer shared without atomic (data race, undefined behavior)
```

### Rule 8 — File Structure

```
src/dsp/     ← NO includes of ui/, visage/, or iPlug2 UI headers
src/ui/      ← may read Types.h / Constants.h from common/
src/common/  ← no dependencies on dsp/ or ui/
```

Violating this makes the DSP test impossible to build without the GUI.

### Rule 9 — Testing

- All DSP tests compile with `-DBUILD_DSP_TEST=ON` and **no** Visage or iPlug2 UI headers.
- Determinism test: run sequencer 10 steps with seed 42, capture output, run again, compare.
- Timing test: verify MIDI offsets are never < 0 or >= nFrames.
- Edge cases to always test:
  - `stepCount = 1`
  - `stepCount = MAX_STEPS (64)`
  - `tempo = 20 BPM`
  - `tempo = 300 BPM`
  - `nFrames = 1`
  - `nFrames = 4096`
  - `probability = 0.0f` (step always silent)
  - `probability = 1.0f` (step always active)
  - `euclidean hits = 0`
  - `euclidean hits = n` (all steps active)

### Rule 10 — Common Agent Mistakes

| # | Mistake | Correct Approach |
|---|---------|------------------|
| 1 | `SendParameterValueFromUI(k, rawValue)` | Pass normalized 0–1 value |
| 2 | Allocating memory in `ProcessBlock` | Pre-allocate in constructor |
| 3 | Using `std::random_device` in audio thread | `mRng` with user seed only |
| 4 | Sending MIDI at offset 0 always | Compute exact `sampleOffset` |
| 5 | Not tracking Note-Offs | Always use `NoteTracker` |
| 6 | Omitting `Begin/EndInformHostOfParamChange` | Always wrap UI parameter changes |
| 7 | Creating `visage::Font` every frame | Create as member in constructor |
| 8 | Using `std::function` in hot path | Use `visage::CallbackList` |
| 9 | Computing fractals in `ProcessBlock` | Pre-compute in UI thread, atomic swap |
| 10 | Defining `EParams` in two headers | Single definition in `src/common/Params.h` |

---

## Build Commands

### DSP Test (no dependencies, fast, CI-safe)

```bash
mkdir build && cd build
cmake -DBUILD_DSP_TEST=ON -DBUILD_VST3=OFF -DBUILD_STANDALONE=OFF ..
cmake --build .
ctest --output-on-failure
```

### Full Plugin Build

```bash
./scripts/setup_deps.sh          # clone iPlug2 + Visage (first time)
mkdir build && cd build
cmake -DBUILD_DSP_TEST=ON -DBUILD_VST3=ON -DBUILD_STANDALONE=ON \
      -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

### Run DSP Test Manually

```bash
./build/bin/NeuroBoost_DSP_Test  # should print PASS for all tests
```

---

## 10-Point Pre-Commit Checklist

Before every commit, verify:

```
[ ] 1. Is this code called in the audio thread?
       → Check for ANY of the FORBIDDEN operations in Rule 1.

[ ] 2. Any new heap allocations?
       → Move to constructor or pre-allocated member array.

[ ] 3. Any new random number usage?
       → Must use mRng (std::mt19937 with user seed), never random_device.

[ ] 4. MIDI events sent?
       → Note tracked in NoteTracker? Sample offset computed correctly?

[ ] 5. Parameter read or written?
       → Is the value normalized (0–1) when calling SendParameterValueFromUI?

[ ] 6. UI code changed?
       → No allocations in draw()? Fonts stored as members?

[ ] 7. Edge cases covered?
       → stepCount=1/64, prob=0/1, tempo=20/300, nFrames=1/4096 tested?

[ ] 8. DSP test compiles without UI?
       → cmake -DBUILD_DSP_TEST=ON -DBUILD_VST3=OFF must succeed.

[ ] 9. Deterministically testable?
       → Same seed produces same sequence in automated test?

[ ] 10. State serialization versioned?
        → Added new params? Increment chunk version, add migration.
```

---

## Cross-References

- **Architecture details:** [ARCHITECTURE.md](ARCHITECTURE.md)
- **Current task backlog:** [STATUS.md](STATUS.md)
- **Past mistakes:** [LESSONS_LEARNED.md](LESSONS_LEARNED.md)
- **Session workflow:** [DEVELOPMENT_WORKFLOW.md](DEVELOPMENT_WORKFLOW.md)
- **UI component specs:** [UI_UX_SPEC.md](UI_UX_SPEC.md)
