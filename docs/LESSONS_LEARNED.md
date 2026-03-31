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
