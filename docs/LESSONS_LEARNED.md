# NeuroBoost – Lessons Learned

> **This is a living document. Every coding agent session must:**
> 1. **Read this entire document before starting work** (Phase 1 of PITD)
> 2. **Append at least one entry at the end of the session** (Phase 4 of PITD)
>
> Do not delete or edit past entries. Only append new ones.

---

## Purpose

This document captures concrete mistakes, their root causes, and prevention
strategies discovered during development. It prevents the same errors from
recurring across different agent sessions.

---

## Entry Format

```
### [DATE] Session: [TOPIC]

**What happened:**
Concrete description of the problem or mistake.

**Root cause:**
Why it happened (misunderstanding, API misuse, missing knowledge).

**Lesson:**
The key takeaway in one sentence.

**Prevention:**
The rule or check to prevent recurrence (add to pre-commit checklist if needed).
```

---

## Categories

- **Architecture** – structural decisions, dependency rules
- **Realtime Safety** – audio thread violations
- **iPlug2 Specifics** – iPlug2 API misuse
- **Visage UI** – Visage API misuse
- **Build System** – CMake, CI, dependency issues
- **Testing** – test coverage gaps, test fragility

---

## Entries

### [2025-03-31] Session: Initial Gain Plugin Analysis

**Category:** iPlug2 Specifics

**What happened:**
`SendParameterValueFromUI(kGain, gainValue)` was called with the raw gain
value (0–200) instead of the normalized value (0.0–1.0). The iPlug2 API
requires a normalized value; passing the raw value caused the host/DAW to
receive wildly out-of-range automation data and broke preset save/recall.

**Root cause:**
The developer did not read the iPlug2 documentation for
`SendParameterValueFromUI`. The method name does not hint at the normalization
requirement. The bug is silent at compile time and produces incorrect behavior
that is easy to miss in casual testing.

**Lesson:**
`SendParameterValueFromUI` always takes a value in [0.0, 1.0]. Use
`GetParam(k)->GetNormalized()` or `rawValue / kParamMax` before passing.

**Prevention:**
Pre-commit checklist item 5: "Is the value normalized when calling
`SendParameterValueFromUI`?" See [AGENTS.md Rule 5](AGENTS.md#rule-5--parameter-handling).

---

### [2025-03-31] Session: Initial Gain Plugin Analysis

**Category:** Realtime Safety

**What happened:**
`ProcessBlock` read `GetParam(kGain)->Value()` once per block and applied it
to all samples without interpolation. During rapid knob movement or host
automation, this produced an audible zipper noise (discontinuous gain steps
at block boundaries).

**Root cause:**
Single-sample-accurate parameter smoothing was not implemented. The developer
assumed block-level parameter updates were sufficient, which is only acceptable
for parameters that change slowly.

**Lesson:**
Always smooth parameter values sample-by-sample in the audio thread, or use
iPlug2's built-in smoothing helpers. A single multiply-per-block is never
sufficient for parameters that can change rapidly.

**Prevention:**
Pre-commit checklist: any parameter applied inside `ProcessBlock` must use
linear interpolation or a first-order IIR smoother. Budget: one additional
member variable `mGainSmoothed` per smoothed parameter.

---

### [2025-03-31] Session: Initial Gain Plugin Analysis

**Category:** iPlug2 Specifics

**What happened:**
`OnIdle()` was left empty. When the host changed the gain parameter via
automation, the knob in the UI did not update. The displayed value was
permanently frozen at the last user-dragged position.

**Root cause:**
`OnIdle()` is the correct place to poll parameter changes and push them to
UI controls. Without this polling, the UI diverges from the actual plugin
state whenever the host is in control.

**Lesson:**
`OnIdle()` must poll all relevant parameters and update UI controls. For the
MIDI sequencer, it must also drain the SPSC event queue from the audio thread.

**Prevention:**
Never leave `OnIdle()` empty in a plugin with a UI. Minimum implementation:
sync all displayed parameters from their `GetParam(k)->GetNormalized()` values.

---

### [2025-03-31] Session: Initial Gain Plugin Analysis

**Category:** Architecture

**What happened:**
`EParams` was defined in both `src/NeuroBoost.h` (for the full plugin) and
`src/NeuroBoostDSP.h` (for the DSP-only test). If both headers were ever
included in the same translation unit, this would cause an ODR (One Definition
Rule) violation or a compile error.

**Root cause:**
The headers were written independently without a shared common header.
The desire to keep `NeuroBoostDSP.h` self-contained led to duplicating
the enum.

**Lesson:**
Shared types and enums belong in a dedicated header (e.g., `src/common/Params.h`)
that both the full plugin and the DSP test can include safely.

**Prevention:**
File structure rule: `EParams` and all shared types live only in `src/common/`.
`src/dsp/` and `src/NeuroBoost.h` both include from `src/common/`. See
[ARCHITECTURE.md – Target File Structure](ARCHITECTURE.md#target-file-structure).

---

### [2025-03-31] Session: Initial Gain Plugin Analysis

**Category:** Visage UI

**What happened:**
The draw callback created a `std::ostringstream` and formatted text into it
every frame. `std::ostringstream` allocates heap memory on construction and
on every `<<` operation involving strings or certain numeric types.

**Root cause:**
The developer was not aware that standard string streaming operations allocate
on the heap. In a 60 FPS draw callback this means hundreds of allocations per
second, increasing GC pressure and potentially causing frame drops.

**Lesson:**
Never use `std::ostringstream`, `std::string`, or any heap-allocating type
in a Visage `draw(Canvas&)` callback. Use pre-allocated char arrays with
`snprintf` or pre-computed string members.

**Prevention:**
Pre-commit checklist item 6: "No allocations in draw()?"
Use `snprintf` into a stack buffer if dynamic text is needed:
```cpp
char buf[32];
snprintf(buf, sizeof(buf), "%.1f%%", value * 100.0);
canvas.text(buf, font, bounds());
```

---

### [2025-03-31] Session: Initial Gain Plugin Analysis

**Category:** Visage UI

**What happened:**
A `visage::Font` object was constructed inside the `draw(Canvas&)` method,
creating a new font resource every frame (60 times per second). This wasted
GPU resources and caused unnecessary shader/texture uploads.

**Root cause:**
Font objects in Visage are non-trivial GPU resources. The developer used the
same pattern as stack-allocated plain structs, not knowing that `visage::Font`
triggers resource registration.

**Lesson:**
`visage::Font` objects must be created once (in the constructor or as class
members) and reused across all frames. The same rule applies to any Visage
resource type that wraps a GPU resource.

**Prevention:**
Pre-commit checklist item 6: "Fonts stored as members, not created in draw()?"
Every Visage component class must declare its fonts as member variables.

---

### [2026-03-31] Session: Sprint 2 – L-System Double-Buffer Technique

**Category:** Realtime Safety / Architecture

**What happened:**
The L-System algorithm must expand strings exponentially (e.g., Fibonacci word
grows as Fib(n) characters per iteration). Using `std::string` would require
heap allocation for each expansion, violating realtime-safety rules.

**Root cause:**
The natural implementation uses `std::string` for string rewriting but the
audio engine must be allocation-free.

**Lesson:**
Use two `static char[MAX_LSYSTEM_LENGTH]` buffers and swap the read/write
pointers at each iteration. Truncate at MAX_LSYSTEM_LENGTH. The `static`
keyword ensures the buffers live in BSS (zero-cost, no stack pressure), while
still being safe since the function is only called from the control thread.

**Prevention:**
Any algorithm that produces a growing string must use a fixed-size double-buffer
approach. Never use `std::string`, `std::vector<char>`, or any STL container
that dynamically allocates inside `src/dsp/`.

---

### [2026-03-31] Session: Sprint 2 – Fractal Computation Caching Strategy

**Category:** Realtime Safety / Performance

**What happened:**
The Mandelbrot set iteration is O(steps × maxIter) per call. Calling it inside
`processBlock` every audio callback at 256-sample buffer size with 48 kHz would
execute >180 times per second, making it prohibitively expensive.

**Root cause:**
The fractal parameters (cx, cy, zoom, maxIter, threshold) only change when the
user moves a control, not on every audio block. There is no need to recompute
the pattern every block.

**Lesson:**
Cache fractal results in `mFractalIterCounts[MAX_STEPS]` and
`mEuclideanPattern[MAX_STEPS]`. Recompute only in `regeneratePattern()`, which
is called from `setFractalParams()` (control thread) and `setGenerationMode()`.
Never call `generateFractal()` from `processBlock()`.

**Prevention:**
Any algorithm that is O(n × iterations) must be "pull-based": compute on
parameter change, cache result, read from cache in the audio thread. Add a
comment in the header: "IMPORTANT: Call from control thread only."

---

### [2026-03-31] Session: Sprint 2 – Markov Absorbing State Handling

**Category:** Algorithm Correctness

**What happened:**
A Markov transition matrix row that sums to 0 represents an "absorbing state"
where no transition is possible. A naive implementation would attempt to divide
by zero when normalizing the probability distribution.

**Root cause:**
The Markov chain implementation computed `r / rowSum` to sample from the
cumulative distribution. If `rowSum == 0`, this produces NaN or a divide-by-zero.

**Lesson:**
Always check `rowSum <= 0` before normalizing. If the row is zero, fall back
to a uniform distribution across all 12 pitch classes. This prevents crashes
with user-supplied matrices and all-zero rows.

**Prevention:**
Add a `rowSum <= 0` guard in `generateMarkov`. Test explicitly: pass an
all-zero 12×12 matrix and verify the output is still valid pitch classes [0,11].

---

## Sprint 5 Lessons

### Lesson S5-1: 1-Pole IIR for Realtime Parameter Smoothing

**Category:** Realtime Safety / Audio Quality

**What happened:**
Moving knobs caused audible "zipper noise" because raw parameter values changed
instantly in the audio thread.

**Lesson:**
A simple 1-pole IIR smoother (`current += coeff * (target - current)`) eliminates
zipper noise with near-zero CPU cost. Choosing ~15 Hz cutoff gives ~64ms glide —
smooth enough for knob automation, fast enough for performance use.

**Key rules:**
- Smoother lives in DSP layer only (`src/dsp/ParamSmoother.h`)
- `setTarget()` is called from the control thread; `process()` runs in the audio thread
- `reset()` snaps immediately (e.g., on transport reset — no glide after jump)
- Recalculate coefficient in `setSampleRate()` to stay accurate across sample rates

---

### Lesson S5-2: Host Automation Sync (OnIdle Polling)

**Category:** Plugin Integration

**What happened:**
When a DAW automated a parameter via a ramp, the UI knob stayed frozen at its
initial position because `OnIdle()` only polled the playhead queue.

**Lesson:**
`OnIdle()` must iterate ALL parameter indices and call `knob->setValueFromHost()`
for each. Use a separate `setValueFromHost()` that updates the visual without
re-firing the `onValueChange` callback (which would create an automation feedback loop).

---

### Lesson S5-3: Always Begin/End InformHostOfParamChange

**Category:** Plugin Integration / DAW Automation

**What happened:**
DAW automation recording did not work because `SendParameterValueFromUI` was
called without the required `BeginInformHostOfParamChange` / `EndInformHostOfParamChange` wrapper.

**Lesson:**
Every UI-initiated parameter change must be wrapped:
```
BeginInformHostOfParamChange(idx);
GetParam(idx)->Set(value);
SendParameterValueFromUI(idx, GetParam(idx)->GetNormalized());
EndInformHostOfParamChange(idx);
```
This tells the DAW that a user gesture started and ended, enabling proper automation lane recording.

---

### Lesson S5-4: Panic Note-Offs for Hung Notes

**Category:** MIDI Safety

**What happened:**
When the DAW stopped transport or the plugin window was closed while notes were
playing, the notes hung indefinitely because no Note-Off was sent.

**Lesson:**
Three hooks are needed for complete panic coverage:
1. `OnActivate(false)` — host bypasses the plugin
2. `CloseWindow()` — UI is torn down
3. `processBlock()` transport-stop transition — isPlaying goes false

In all cases: call `panicAllNotes()` on the NoteTracker, then send `IMidiMsg::MakeNoteOffMsg`
for each returned active note BEFORE resetting engine state.

---

### Lesson S5-5: Structural UI Tests Without GPU

**Category:** Testing Strategy

**What happened:**
Visage UI requires GPU rendering which is unavailable in CI environments.
A "pixel-perfect screenshot" approach would fail in headless CI.

**Lesson:**
Separate the "structure" concern from the "rendering" concern:
- **Structure tests** (runnable in CI): mock the component tree with plain C++ structs,
  verify bounds, option counts, callbacks, and state transitions.
- **ASCII dump**: serialize the mock component tree to a text file and store as CI artifact.
  Compare against a checked-in reference on subsequent runs.
- This provides high confidence that UI logic is correct without GPU dependency.

