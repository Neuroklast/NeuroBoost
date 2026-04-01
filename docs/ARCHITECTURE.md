# NeuroBoost – Technical Architecture

> **NeuroBoost** is a deterministic & probabilistic MIDI sequencer plugin built with
> **iPlug2** (cross-platform audio plugin framework) and **Visage** (GPU-accelerated UI).
> This document is the authoritative technical reference for all development.

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Realtime Safety Rules](#realtime-safety-rules)
3. [Thread Communication](#thread-communication)
4. [Data Model](#data-model)
5. [Timing Architecture](#timing-architecture)
6. [Note-Off Management](#note-off-management)
7. [Determinism Guarantee](#determinism-guarantee)
8. [MIDI Input Architecture](#midi-input-architecture)
9. [Pattern Export (MIDI File)](#pattern-export-midi-file)
10. [Keyboard Shortcuts](#keyboard-shortcuts)
11. [Mathematical Algorithms](#mathematical-algorithms)
12. [Target File Structure](#target-file-structure)

---

## System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                        HOST / DAW PROCESS                           │
│                                                                     │
│  ┌─────────────────────────┐     ┌───────────────────────────────┐  │
│  │     REALTIME THREAD     │     │         UI THREAD             │  │
│  │    (Audio Callback)     │     │          (Visage)             │  │
│  │                         │     │                               │  │
│  │  SequencerEngine        │     │  EditorView                   │  │
│  │  ├─ ProcessBlock()      │     │  ├─ draw(Canvas&)             │  │
│  │  ├─ Algorithms          │     │  ├─ StepGrid                  │  │
│  │  ├─ NoteTracker         │     │  ├─ PresetBrowser             │  │
│  │  ├─ TransportSync       │     │  ├─ ModeSelector (gen+MIDI)   │  │
│  │  └─ handleMidiInput()   │     │  ├─ ParameterPanel            │  │
│  │                         │     │  ├─ keyDown() shortcuts       │  │
│  │                         │     │  └─ OnIdle() poll             │  │
│  └────────┬────────────────┘     └──────────────┬────────────────┘  │
│           │                                     │                   │
│           │  iPlug2 Parameter System            │                   │
│           │  (thread-safe bridge)               │                   │
│           │◄────────────────────────────────────┤                   │
│           │  SendParameterValueFromUI            │                   │
│           │  (UI → Audio, NORMALIZED value)      │                   │
│           │                                     │                   │
│           │  Lock-Free SPSC Ring Buffer          │                   │
│           ├────────────────────────────────────►│                   │
│           │  (Audio pushes, UI polls OnIdle)     │                   │
│           │                                     │                   │
│           │  Double-Buffer + atomic swap         │                   │
│           ├────────────────────────────────────►│                   │
│           │  (complex data: pattern updates)     │                   │
└───────────┴─────────────────────────────────────┴───────────────────┘
```

### Key Design Principles

- The **Realtime Thread** never blocks, never allocates heap memory, never calls OS services.
- The **UI Thread** owns all visual state, polls the audio thread on `OnIdle()`.
- The **iPlug2 Parameter System** is the only safe synchronous bridge (UI → Audio).
- **Lock-free SPSC Ring Buffers** carry asynchronous events (Audio → UI).
- **Double-buffering with atomic swap** carries complex read-only snapshots (Audio → UI).

---

## Realtime Safety Rules

### FORBIDDEN in the Audio Thread

| Category | Forbidden Operations |
|---|---|
| Memory | `new`, `delete`, `malloc`, `free`, `realloc` |
| STL Containers | `std::vector::push_back`, `std::deque`, any growing container |
| Strings | `std::string` (constructors can allocate), `std::ostringstream` |
| Synchronization | `std::mutex::lock`, `std::condition_variable::wait`, any blocking primitive |
| I/O | `std::cout`, `std::cerr`, `printf`, any file I/O, `fopen`, `std::fstream` |
| Maps | `std::map` inserts/erases, `std::unordered_map` inserts/erases |
| Filesystem | `std::filesystem::*`, `open()`, `stat()` |
| Exceptions | `throw`, any function with `throw` in its contract |
| Function wrappers | `std::function` (can heap-allocate closure) |
| OS entropy | `std::random_device`, `time()`, `clock()` |
| Dynamic dispatch | Virtual function calls on objects with potential allocation in vtable setup |

### ALLOWED in the Audio Thread

| Category | Allowed Operations |
|---|---|
| Memory | Fixed-size stack arrays, pre-allocated member arrays |
| Atomics | `std::atomic<T>` load/store/compare_exchange (lock-free specializations) |
| Queues | Lock-free SPSC ring buffers (pre-allocated) |
| Math | All `<cmath>` operations, SIMD intrinsics, bit-twiddling |
| RNG | `std::mt19937` (deterministic, no heap, no OS calls) |
| Buffers | Pre-allocated fixed-size buffers, `memcpy`, `memset` |
| Control flow | `switch`, `if`, ternary, loops over fixed-size arrays |
| iPlug2 | `GetParam(kX)->Value()`, `SendMidiMsg()`, `GetSampleRate()`, `GetTempo()` |

---

## Thread Communication

### UI → Audio: iPlug2 Parameter System

```cpp
// In UI callback (UI thread):
// ALWAYS pass NORMALIZED value (0.0 to 1.0)
double normalizedValue = rawValue / kParamMaxValue;
BeginInformHostOfParamChange(kMyParam);
SendParameterValueFromUI(kMyParam, normalizedValue);  // normalized!
EndInformHostOfParamChange(kMyParam);

// In audio thread (ProcessBlock):
double rawValue = GetParam(kMyParam)->Value();  // iPlug2 converts internally
```

### Audio → UI: Lock-Free SPSC Ring Buffer

```cpp
// Audio thread pushes (non-blocking, always succeeds or drops):
SequencerEvent ev;
ev.type = SequencerEvent::StepTriggered;
ev.stepIndex = mCurrentStep;
ev.beatPos  = mCurrentBeat;
mEventQueue.push(ev);  // SPSC, pre-allocated, lock-free

// UI thread polls in OnIdle():
void NeuroBoost::OnIdle() {
    SequencerEvent ev;
    while (mEventQueue.pop(ev)) {
        // update UI state, trigger animations
        mEditor->onSequencerEvent(ev);
    }
}
```

### Complex Data: Double-Buffering with Atomic Swap

```cpp
// Audio thread writes to back buffer, then swaps atomically:
PatternSnapshot& back = mPatternBuffers[mWriteIndex];
back = mCurrentPattern;  // copy into pre-allocated struct
mReadIndex.store(mWriteIndex, std::memory_order_release);
mWriteIndex ^= 1;  // flip 0/1

// UI thread reads from front buffer (never blocks):
int idx = mReadIndex.load(std::memory_order_acquire);
const PatternSnapshot& snap = mPatternBuffers[idx];
// render snap safely
```

### Rule: NEVER Shared Mutable State Without Synchronization

- No raw pointer shared between threads without atomic or lock-free guard.
- No `static` mutable variables accessible from both threads.
- No global variables modified on one thread and read on another.

---

## Data Model

All state is **pre-allocated at construction time**. No dynamic allocation after `Reset()`.

```cpp
// Constants (in src/common/Constants.h)
constexpr int MAX_STEPS        = 64;
constexpr int MAX_POLYPHONY    = 32;
constexpr int MAX_LSYSTEM_LEN  = 1024;
constexpr int MARKOV_ORDER_MAX = 3;
constexpr int MATRIX_SIZE      = 12;   // chromatic scale

// ---- Step data ------------------------------------------------
struct StepData {
    bool   active         = false;
    int    pitch          = 60;       // MIDI note number
    float  velocity       = 0.8f;     // 0.0–1.0
    float  velocityRange  = 0.0f;     // random variation ±
    float  probability    = 1.0f;     // 0.0–1.0
    float  duration       = 0.5f;     // fraction of step length
    int    ratchetCount   = 1;        // 1 = no ratchet
    float  ratchetDecay   = 0.8f;     // velocity decay per ratchet
    int    conditionMode  = 0;        // 0=always,1=fill,2=not-fill,…
    int    conditionParam = 0;
    float  microTiming    = 0.0f;     // ±0.5 steps offset
};

// ---- Global sequencer parameters ------------------------------
struct GlobalParams {
    int   stepCount      = 16;
    int   scaleMode      = 0;         // index into scale table
    int   rootNote       = 60;        // MIDI C4
    int   octaveRange    = 2;
    float swing          = 0.0f;      // 0.0–1.0
    float density        = 0.5f;      // 0.0–1.0
    int   generationMode = 0;         // 0=Euclidean,1=LSystem,…
    int   transportMode  = 0;         // 0=host,1=internal
};

// ---- MIDI Input (Sprint 7) -----------------------------------
enum class MidiInputMode : int {
    Off = 0,          // Ignore incoming MIDI (default)
    Thru,             // Pass MIDI through unchanged
    Transpose,        // Use incoming note as transpose offset
    Trigger,          // Incoming note-on triggers pattern restart
    Gate,             // Pattern only plays while note held
    ThruAndGenerate   // Both pass-through AND generate patterns
};

// ---- Algorithm-specific parameter blocks ----------------------
struct FractalParams {
    uint32_t seed        = 42;
    float    dimension   = 1.5f;
    int      depth       = 4;
    float    mutationRate= 0.1f;
    float    cx          = -0.7f;     // Julia set real part
    float    cy          =  0.27f;    // Julia set imaginary part
    int      mappingMode = 0;         // 0=pitch,1=velocity,2=both
};

struct EuclideanParams {
    int hits     = 4;
    int steps    = 16;
    int rotation = 0;
    int layers   = 1;
};

struct MarkovParams {
    int   order      = 1;
    float transitionMatrix[MATRIX_SIZE][MATRIX_SIZE] = {};  // pre-allocated!
    float temperature = 1.0f;
};

// ---- Runtime state (audio thread only, never shared) ----------
struct ActiveNote {
    int   pitch       = -1;
    float noteOffBeat = -1.0f;
    int   channel     = 0;
};

struct RuntimeState {
    int        currentStep  = 0;
    double     currentBeat  = 0.0;
    bool       isPlaying    = false;
    uint64_t   cycleCount   = 0;
    ActiveNote activeNotes[MAX_POLYPHONY] = {};
    int        activeNoteCount = 0;
};

// ---- Complete sequencer state ---------------------------------
struct SequencerState {
    GlobalParams   globalParams;
    FractalParams  fractalParams;
    EuclideanParams euclideanParams;
    MarkovParams   markovParams;
    StepData       steps[MAX_STEPS];
    // RuntimeState is audio-thread-only, NOT part of serialized state
};
```

---

## Timing Architecture

### Per-Block Information from Host

```cpp
void NeuroBoost::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
    const auto& ti = GetTimeInfo();
    bool   isPlaying    = ti.mTransportIsPlaying;
    double ppqPos       = ti.mPPQPos;       // beats since song start
    double tempo        = ti.mTempo;        // BPM
    double sampleRate   = GetSampleRate();
}
```

### Sample-Accurate MIDI Scheduling

Steps can fall **within** a block. MIDI Note-On must be sent at the **exact sample offset**:

```cpp
// For each step that falls within [blockStartBeat, blockStartBeat + blockLengthBeats):
double beatsPerSample = tempo / (60.0 * sampleRate);
double blockLengthBeats = nFrames * beatsPerSample;
double stepBeatPosition = stepIndex * stepLengthInBeats;

if (stepBeatPosition >= blockStartBeat &&
    stepBeatPosition <  blockStartBeat + blockLengthBeats) {

    int sampleOffset = static_cast<int>(
        (stepBeatPosition - blockStartBeat) / beatsPerSample
    );
    sampleOffset = std::clamp(sampleOffset, 0, nFrames - 1);

    IMidiMsg noteOn;
    noteOn.MakeNoteOnMsg(pitch, velocity, sampleOffset);
    SendMidiMsg(noteOn);
}
```

**NOT:** `noteOn.MakeNoteOnMsg(pitch, velocity, 0)` — sending everything at offset 0
is audibly inaccurate at moderate/high tempos.

### Swing Timing

```cpp
// Odd steps (1, 3, 5, ...) are delayed by swing amount
double swingOffset = 0.0;
if (stepIndex % 2 == 1) {
    // swing = 0.0 → no delay, swing = 1.0 → delayed by half a step
    swingOffset = swing * stepLengthInBeats * 0.5;
}
double stepBeatPosition = stepIndex * stepLengthInBeats + swingOffset;
```

---

## Note-Off Management

Every Note-On **must** have a guaranteed Note-Off. Use a pre-allocated tracker:

```cpp
// Register on Note-On:
void NoteTracker::registerNote(int pitch, int channel, float noteOffBeat) {
    for (int i = 0; i < MAX_POLYPHONY; ++i) {
        if (mNotes[i].pitch == -1) {          // find empty slot
            mNotes[i] = {pitch, channel, noteOffBeat};
            ++mActiveCount;
            return;
        }
    }
    // Polyphony exceeded: steal oldest note (send Note-Off immediately)
    sendNoteOffForSlot(0);
    mNotes[0] = {pitch, channel, noteOffBeat};
}

// Check every ProcessBlock:
void NoteTracker::processBlock(double blockStartBeat, double blockEndBeat,
                                double beatsPerSample, int nFrames) {
    for (int i = 0; i < MAX_POLYPHONY; ++i) {
        if (mNotes[i].pitch == -1) continue;
        if (mNotes[i].noteOffBeat >= blockStartBeat &&
            mNotes[i].noteOffBeat <  blockEndBeat) {
            int offset = static_cast<int>(
                (mNotes[i].noteOffBeat - blockStartBeat) / beatsPerSample
            );
            offset = std::clamp(offset, 0, nFrames - 1);
            IMidiMsg noteOff;
            noteOff.MakeNoteOffMsg(mNotes[i].pitch, offset, mNotes[i].channel);
            SendMidiMsg(noteOff);
            mNotes[i].pitch = -1;
            --mActiveCount;
        }
    }
}

// Panic: send ALL active Note-Offs (on stop / bypass / close / reset):
void NoteTracker::panicAllNotesOff(int sampleOffset) {
    for (int i = 0; i < MAX_POLYPHONY; ++i) {
        if (mNotes[i].pitch != -1) {
            IMidiMsg noteOff;
            noteOff.MakeNoteOffMsg(mNotes[i].pitch, sampleOffset, mNotes[i].channel);
            SendMidiMsg(noteOff);
            mNotes[i].pitch = -1;
        }
    }
    mActiveCount = 0;
}
```

Panic must be called on:
- Transport stop (`isPlaying` false → true transition checked each block)
- Plugin bypass engaged
- `OnReset()` / plugin close

---

## Determinism Guarantee

```cpp
// In SequencerEngine (audio thread):
std::mt19937 mRng;  // pre-allocated, no heap

// Re-seed on every transport start (false → true):
void SequencerEngine::onTransportStart(uint32_t userSeed) {
    mRng.seed(userSeed);  // deterministic from user-settable seed
    mCycleCount = 0;
}

// ALL probabilistic decisions use ONLY mRng:
float prob = std::uniform_real_distribution<float>(0.0f, 1.0f)(mRng);
int   note = std::uniform_int_distribution<int>(0, 11)(mRng);

// Mutation: derive new seed from RNG (reproducible chain):
void SequencerEngine::mutate() {
    uint32_t newSeed = mRng();  // deterministic next value
    mRng.seed(newSeed);
    mState.fractalParams.seed = newSeed;
}

// NEVER:
// std::random_device rd; mRng.seed(rd());   // non-deterministic!
// mRng.seed(time(nullptr));                 // non-deterministic!
// rand()                                    // global state, not seeded
```

---

## MIDI Input Architecture

Sprint 7 adds 6 MIDI input modes, configured by the `kMidiInputMode` plugin parameter.
MIDI input is processed in the audio thread via `NeuroBoost::handleMidiInput()`.

### MidiInputMode Enum (Types.h)

| Mode | Behavior |
|------|----------|
| **Off** | Ignore incoming MIDI (default) |
| **Thru** | Pass MIDI through unchanged |
| **Transpose** | Incoming note sets transpose offset (`noteNumber - 60`) |
| **Trigger** | Note-on triggers `resetPlayhead()` (pattern restart) |
| **Gate** | Pattern only plays while at least one note is held |
| **ThruAndGenerate** | Both pass-through AND generate patterns |

### SequencerEngine Support

```cpp
// Set semitone transpose offset (applied AFTER scale quantization, BEFORE octave clamp)
void SequencerEngine::setTransposeOffset(int semitones);

// Reset playhead to step 0 without regenerating the pattern
void SequencerEngine::resetPlayhead();
```

### Thread Safety

- `IMidiQueue` drains incoming MIDI in `ProcessBlock()` (audio thread).
- `handleMidiInput()` calls engine methods directly (all audio-thread-safe).
- Gate mode tracks `mHeldNoteCount` (audio-thread-only state, no synchronization needed).

---

## Pattern Export (MIDI File)

`MidiExport` (`src/common/MidiExport.h/cpp`) writes Standard MIDI Files (SMF Format 0).
This runs on the **UI thread** only — it uses file I/O, heap allocation, and STL containers.

### Public API

```cpp
struct MidiExport::ExportParams {
    const SequenceStep* steps;      // Pointer to sequence steps
    int    stepCount;               // Number of steps (1..MAX_STEPS)
    double bpm;                     // Tempo in beats per minute
    int    ticksPerQuarterNote;     // SMF resolution (default 480)
    int    rootNote;                // Base MIDI note (0..127)
    ScaleMode scaleMode;            // For display (not used in export)
};

static bool MidiExport::writeToFile(const char* filePath, const ExportParams& params);
```

### SMF Format 0 Structure

- MThd chunk: format 0, 1 track, configurable ticks-per-quarter-note
- Tempo meta event (FF 51 03)
- Time signature meta event (4/4)
- Note-On/Note-Off events with delta times (VLQ encoded)
- Ratchet support: ratcheted steps emit multiple sub-notes with velocity decay
- End-of-track meta event

---

## Keyboard Shortcuts

`EditorView::keyDown()` handles keyboard shortcuts for power-user workflow.

| Key | Action |
|-----|--------|
| **Space** | Toggle play/pause |
| **1–7** | Switch generation mode (Euclidean, Fibonacci, …, Probability) |
| **← / →** | Navigate selected step (wraps around) |
| **↑** | Increase velocity on selected step (+10%) |
| **↓** | Decrease velocity on selected step (-10%) |
| **Delete / Backspace** | Deactivate selected step |
| **A** | Toggle accent on selected step |
| **R** | Randomize (bump fractal seed) |

---

## Mathematical Algorithms

### Euclidean Rhythms (Bjorklund Algorithm)

```cpp
// Distribute k hits as evenly as possible over n steps.
// Edge cases:
//   k > n  → clamp k = n (every step is active)
//   k == 0 → all steps silent
//   n == 0 → undefined; return empty pattern
//   rotation → safe modulo: ((rotation % n) + n) % n

void euclidean(int k, int n, int rotation, bool* out) {
    if (n <= 0) return;
    k = std::clamp(k, 0, n);
    int rot = ((rotation % n) + n) % n;

    // Bjorklund: build pattern with Euclidean distribution
    int bucket = 0;
    for (int i = 0; i < n; ++i) {
        bucket += k;
        out[(i + rot) % n] = (bucket >= n);
        if (bucket >= n) bucket -= n;
    }
}
```

### L-Systems (Lindenmayer)

```cpp
// WARNING: L-Systems grow exponentially! Pre-compute in UI thread only.
// MAX_LSYSTEM_LENGTH = 1024 enforced at every expansion step.

constexpr int MAX_LSYSTEM_LENGTH = 1024;

// Do NOT run this in the audio thread.
// Result is placed in double-buffer, swapped atomically to audio thread.
void expandLSystem(const char* axiom, const LRule rules[], int numRules,
                   int depth, char* out, int& outLen) {
    // ... Bjorklund-style string rewriting, capped at MAX_LSYSTEM_LENGTH
}
```

### Cellular Automata (Wolfram Rules)

```cpp
// Elementary 1D CA with toroidal (wrap-around) boundary.
// Interesting rules for musical rhythms: 30, 54, 90, 110, 150, 182

void caStep(const bool* current, bool* next, int n, uint8_t rule) {
    for (int i = 0; i < n; ++i) {
        int left   = current[(i - 1 + n) % n];  // wrap-around
        int center = current[i];
        int right  = current[(i + 1) % n];
        int index  = (left << 2) | (center << 1) | right;
        next[i] = (rule >> index) & 1;
    }
}
// Pre-allocate grid[2][MAX_STEPS]; double-buffer ping-pong.
```

### Fractal Mapping (Mandelbrot / Julia)

```
WARNING: Fractal computation is CPU-intensive.
Pre-compute in UI thread, swap result to audio thread via atomic.
DO NOT compute fractals inside ProcessBlock.

Julia set iteration (for point (x, y) with constant (cx, cy)):
  z = x + iy
  repeat up to maxIter:
    z = z^2 + c   where c = cx + i*cy
    if |z| > 2.0: escape, record iteration count
  map iteration count → pitch offset / velocity / probability

Sensible default ranges:
  cx: [-1.5,  0.5]   default: -0.7
  cy: [-1.25, 1.25]  default:  0.27
  zoom: [0.1, 10.0]  default: 1.0
  maxIter: 32–256    default: 64
```

### Markov Chains

```cpp
// Transition matrix T[from][to], rows must sum to 1.0.
// Normalization:
void normalizeRow(float row[], int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += row[i];
    if (sum < 1e-6f) {
        // Absorbing state fallback: uniform distribution
        float uniform = 1.0f / n;
        for (int i = 0; i < n; ++i) row[i] = uniform;
        return;
    }
    for (int i = 0; i < n; ++i) row[i] /= sum;
}

// Temperature-scaled sampling:
//   p_i' = p_i^(1/T) / sum(p_j^(1/T))
//   T < 1 → more deterministic (peaks sharper)
//   T > 1 → more random (flatter distribution)
int sampleWithTemperature(const float probs[], int n, float T,
                           std::mt19937& rng) {
    float scaled[MATRIX_SIZE];
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) {
        scaled[i] = std::pow(probs[i] + 1e-9f, 1.0f / T);
        sum += scaled[i];
    }
    float r = std::uniform_real_distribution<float>(0.0f, sum)(rng);
    for (int i = 0; i < n; ++i) {
        r -= scaled[i];
        if (r <= 0.0f) return i;
    }
    return n - 1;  // fallback
}
```

### Scale Quantization

```cpp
// Lookup table for common scales (semitone offsets from root)
// Safe modulo: ((x % n) + n) % n  — handles negative x

constexpr int SCALE_CHROMATIC[]   = {0,1,2,3,4,5,6,7,8,9,10,11};
constexpr int SCALE_MAJOR[]       = {0,2,4,5,7,9,11};
constexpr int SCALE_MINOR_NAT[]   = {0,2,3,5,7,8,10};
constexpr int SCALE_PENTATONIC[]  = {0,2,4,7,9};
constexpr int SCALE_BLUES[]       = {0,3,5,6,7,10};
constexpr int SCALE_WHOLE_TONE[]  = {0,2,4,6,8,10};
// ... (15+ scales total)

int quantizeToScale(int midiNote, const int* scale, int scaleLen, int rootNote) {
    int octave   = midiNote / 12;
    int semitone = ((midiNote - rootNote) % 12 + 12) % 12;
    // Find nearest scale degree
    int nearest = scale[0];
    int minDist = 12;
    for (int i = 0; i < scaleLen; ++i) {
        int dist = std::abs(semitone - scale[i]);
        if (dist > 6) dist = 12 - dist;  // wrap-around distance
        if (dist < minDist) { minDist = dist; nearest = scale[i]; }
    }
    return rootNote + octave * 12 + nearest;
}
```

---

## Target File Structure

```
src/
├── dsp/                        # ONLY realtime-safe code — no UI, no Visage, no iPlug2 UI!
│   ├── SequencerEngine.h       # Main audio-thread sequencer loop
│   ├── SequencerEngine.cpp
│   ├── Algorithms.h            # Euclidean, L-System, CA, Markov, Fractal map
│   ├── Algorithms.cpp
│   ├── ScaleQuantizer.h        # Scale lookup table + quantize()
│   ├── NoteTracker.h           # ActiveNote array, panicAllNotesOff()
│   ├── TransportSync.h         # PPQ tracking, beat-position math
│   └── LockFreeQueue.h         # SPSC ring buffer (header-only template)
│
├── ui/
│   ├── EditorView.h            # Top-level visage::Frame (preset browser, keyboard shortcuts)
│   ├── EditorView.cpp
│   ├── components/
│   │   ├── StepGrid.h/cpp      # 64-step grid with playhead + selection highlight
│   │   ├── StepCell.h/cpp      # Individual step cell (click, drag, right-click, selected state)
│   │   ├── PresetBrowser.h/cpp # Horizontal preset browser (left/right nav, dirty indicator)
│   │   ├── ModeSelector.h/cpp  # Generation mode / MIDI mode dropdown
│   │   ├── ParameterKnob.h/cpp # Per-mode parameter knobs
│   │   ├── TransportBar.cpp    # Play/stop/reset transport
│   │   ├── ProbabilityBar.h/cpp
│   │   ├── Knob.h/cpp          # Reusable knob (wheel, shift-drag, dbl-click reset)
│   │   ├── Selector.h/cpp      # Mode/scale selector
│   │   ├── PianoRoll.h/cpp     # Mini piano roll for pitch editing
│   │   └── FractalView.h/cpp   # GPU-rendered fractal visualizer
│   └── theme/
│       └── Theme.h             # VISAGE_THEME_COLOR macros, font refs, spacing
│
├── common/
│   ├── Params.h                # EParams enum (single definition!)
│   ├── Types.h                 # SequencerState, StepData, MidiInputMode, MidiNote
│   ├── Constants.h             # MAX_STEPS, MAX_POLYPHONY, etc.
│   ├── MidiExport.h            # SMF Format 0 MIDI file writer (UI thread only)
│   └── MidiExport.cpp
│
├── NeuroBoost.h                # Plugin class (includes dsp/ and ui/)
└── NeuroBoost.cpp              # Plugin implementation, OnIdle, ProcessBlock, MIDI input
```

**Dependency rules:**
- `dsp/` must NOT include anything from `ui/`, `visage/`, or iPlug2 UI headers.
- `ui/` may include `dsp/` headers only for read-only data types (`Types.h`, `Constants.h`).
- `common/` has no dependencies on `dsp/` or `ui/`.
- `MidiExport` runs on the **UI thread** only — may use file I/O, heap allocation, STL containers.
