# NeuroBoost – UI/UX Specification

> This document defines the visual layout, interaction patterns, and UX rules
> for the NeuroBoost Visage UI. All UI agents must read this before implementing
> any component.

---

## Default Window Size

**900 × 600 px** (resizable from 400×300 to 1600×1200)

---

## Full Layout Mockup (900×600)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│  HEADER (900 × 48)                                                              │
│  [NeuroBoost]  v0.1.0   [MODE: Euclidean ▼]  [SCALE: Major ▼]  [ROOT: C ▼]   │
│                          generation mode       scale selector    root note     │
├──────────────────────────────────────────┬──────────────────────────────────────┤
│  STEP GRID (600 × 300)                   │  FRACTAL VISUALIZER (300 × 300)     │
│                                          │                                     │
│  ┌──┬──┬──┬──┬──┬──┬──┬──┐             │  ┌─────────────────────────────────┐ │
│  │1 │2 │3 │4 │5 │6 │7 │8 │  row 1      │  │                                 │ │
│  ├──┼──┼──┼──┼──┼──┼──┼──┤             │  │   Julia / Mandelbrot set        │ │
│  │9 │10│11│12│13│14│15│16│  row 2      │  │   real-time GPU render          │ │
│  ├──┼──┼──┼──┼──┼──┼──┼──┤             │  │                                 │ │
│  │17│18│19│20│21│22│23│24│  row 3      │  │   (inactive in Euclidean mode)  │ │
│  ├──┼──┼──┼──┼──┼──┼──┼──┤             │  │                                 │ │
│  │25│26│27│28│29│30│31│32│  row 4      │  └─────────────────────────────────┘ │
│  ├──┼──┼──┼──┼──┼──┼──┼──┤             │                                     │
│  │33│..│..│..│..│..│..│40│  row 5      │  [CX] ──●────────  [-0.700]        │
│  ├──┼──┼──┼──┼──┼──┼──┼──┤             │  [CY] ──────●────  [0.270]         │
│  │41│..│..│..│..│..│..│48│  row 6      │  [DEPTH]  ○○○○     [4]             │
│  ├──┼──┼──┼──┼──┼──┼──┼──┤             │  [SEED]   [42    ] [RESEED]        │
│  │49│..│..│..│..│..│..│56│  row 7      │                                     │
│  ├──┼──┼──┼──┼──┼──┼──┼──┤             │                                     │
│  │57│..│..│..│..│..│..│64│  row 8      │                                     │
│  └──┴──┴──┴──┴──┴──┴──┴──┘             │                                     │
│  [◄ playhead ────────────── ►]          │                                     │
├──────────────────────────────────────────┴──────────────────────────────────────┤
│  PARAMETER PANEL (900 × 252)                                                    │
│                                                                                 │
│  ┌─ RHYTHM ──────────────┐  ┌─ MELODY ──────────────┐  ┌─ PERFORMANCE ──────┐ │
│  │                       │  │                       │  │                    │ │
│  │  [STEPS]  [HITS]      │  │  [OCTAVE]  [DENSITY]  │  │  [SWING]  [TEMPO]  │ │
│  │   ○ 16     ○ 4        │  │    ○ 2      ○ 0.5     │  │   ○ 0.0   ○ 120   │ │
│  │                       │  │                       │  │                    │ │
│  │  [ROTATE] [LAYER]     │  │  [VELOCITY RANGE]     │  │  [PROB]  [RATCHET] │ │
│  │   ○ 0      ○ 1        │  │    ────────●───       │  │   ○ 1.0   ○ 1     │ │
│  │                       │  │                       │  │                    │ │
│  └───────────────────────┘  └───────────────────────┘  └────────────────────┘ │
│                                                                                 │
│  [PRESET: Init ▼]  [LOAD]  [SAVE]  [A/B]          [PANIC]  [BYPASS]           │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## Component Descriptions

### Header (900 × 48)

- **Plugin name** ("NeuroBoost"): left-aligned, bold, accent color.
- **Version string**: small, muted color, right of name.
- **Generation mode selector** (`Selector` component): center-left.
  Options: Euclidean, L-System, Cellular Automata, Markov, Fractal, Random Walk, User.
- **Scale selector**: center. 15+ scale options.
- **Root note selector**: center-right. C through B.

### Step Grid (600 × 300)

- **64 steps** displayed in an 8×8 grid (or 16×4 for stepCount=16).
- Each cell is a `StepCell` component with:
  - Background color reflects **active** state.
  - Brightness reflects **velocity**.
  - Small indicator shows **probability** (dot or fill level).
  - Animated **flash** on trigger (frame-accurate from audio event).
- **Playhead**: a highlighted column moving left-to-right, driven by
  sequencer position updates from the audio thread via SPSC queue.
- Grid layout adapts to `stepCount` (1–64).

### Fractal Visualizer (300 × 300)

- GPU-rendered Julia/Mandelbrot set using Visage canvas drawing.
- Responds to `cx`, `cy`, `depth`, `zoom` parameter changes.
- **Inactive** in non-fractal modes (shows greyed-out overlay).
- **Click and drag** to pan the view.
- **Mouse wheel** to zoom.
- Pre-computed in UI thread; result swapped atomically to audio thread.

### Parameter Panel (900 × 252)

Three groups:

| Group | Parameters |
|-------|-----------|
| **RHYTHM** | Steps (1–64), Hits (Euclidean), Rotation, Layers |
| **MELODY** | Octave range, Density, Velocity range |
| **PERFORMANCE** | Swing, Tempo (internal mode), Probability, Ratchet count |

Bottom row:
- **Preset selector** with Load/Save/A-B comparison.
- **PANIC button**: sends all-notes-off immediately.
- **BYPASS toggle**: disables MIDI output, sends panic on engage.

---

## Interaction Patterns

### Knob

| Interaction | Action |
|-------------|--------|
| Click + drag up/down | Change value (normal sensitivity) |
| Shift + drag | Fine adjustment (10× slower) |
| Double-click | Reset to default value |
| Right-click | Context menu: "Reset", "Enter value", "MIDI Learn" |
| Mouse wheel | Increment/decrement by small step |
| Ctrl + drag | Map to host automation (BeginInformHostOfParamChange) |

### Step Cell

| Interaction | Action |
|-------------|--------|
| Left click | Toggle step active/inactive |
| Left drag (vertical) | Adjust velocity |
| Shift + left drag (vertical) | Adjust probability |
| Right-click | Context menu: "Set pitch", "Set duration", "Copy", "Paste", "Clear" |
| Double-click | Open per-step detail editor (pitch, velocity, probability, duration, ratchet) |

### Fractal Visualizer

| Interaction | Action |
|-------------|--------|
| Click + drag | Pan view |
| Mouse wheel | Zoom in/out |
| Double-click | Reset to default view |
| Right-click | Context menu: "Reset view", "Export image" |

### Selector (dropdown)

| Interaction | Action |
|-------------|--------|
| Click | Open dropdown list |
| Click option | Select and close |
| Mouse wheel over selector | Cycle through options |
| Keyboard arrow keys (focused) | Cycle through options |

---

## Visual Feedback Requirements

### Playhead Animation

- Playhead moves step-to-step, driven by `SequencerEvent::StepTriggered`
  messages from the audio thread (via SPSC queue, polled in `OnIdle()`).
- Current step column is highlighted in accent color.
- Animation is **not** time-interpolated between steps; each update is
  discrete, matching the actual MIDI event.

### Step Trigger Flash

- When a step fires, its cell flashes bright white for ~80ms.
- Brightness decay follows a simple linear fade over the 80ms.
- Implemented via a `flashTimer` member in `StepCell`, reset on trigger,
  decremented each frame.

### Probability Pulse

- Steps with probability < 1.0 show a subtle "breathing" opacity animation
  at ~0.5 Hz to indicate uncertainty.
- The animation pauses when the sequencer is stopped.

### Velocity Brightness

- Cell background brightness scales with velocity (0.0 = dark, 1.0 = full color).
- Color: accent color at full velocity, muted/grey at low velocity.

### Mode Indicator

- When fractal mode is active, the FractalView panel glows with a border in
  accent color.
- Other modes dim the FractalView.

---

## UX Rules

1. **Immediately audible**: every parameter change must take effect within
   one audio block (~5ms at 256-sample / 44100 Hz).

2. **Discoverable**: every control must have a tooltip explaining its function
   and value range.

3. **DAW integration**: all parameters must be automatable. Knob changes
   must call `BeginInformHostOfParamChange` / `EndInformHostOfParamChange`
   so the DAW records the automation.

4. **Undo-friendly**: parameter changes driven by user interaction must be
   wrapped in begin/end pairs so the DAW can undo them.

5. **Preset system**: factory presets must cover each generation mode.
   User presets saved to disk (outside audio thread, on `OnIdle`).

6. **PANIC is always accessible**: the PANIC button must be visible at all
   times and respond within one render frame.

7. **No destructive defaults**: loading a preset or resetting a parameter
   never silently deletes user patterns. Confirm dialogs for destructive
   actions.

8. **Feedback for probabilistic steps**: steps with probability < 1.0 must
   be visually distinguishable from fully-active steps.

---

## Accessibility Requirements

- All interactive controls must be keyboard-navigable (Tab order defined).
- Knob value changes via keyboard: Arrow keys (small step), Shift+Arrow (large step).
- Screen reader text for all controls (use Visage accessibility API when available).
- Sufficient color contrast: minimum 4.5:1 (WCAG AA) for all text.
- Color is never the sole indicator of state (add shape/pattern differentiation).

---

## Frame Budget (60 FPS Target)

| Component | Budget | Notes |
|-----------|--------|-------|
| StepGrid | 2 ms | 64 cells, simple rectangles |
| FractalView | 8 ms | GPU-rendered, pre-computed |
| ParameterPanel | 1 ms | Knobs, sliders, selectors |
| Header | 0.5 ms | Text + dropdowns |
| Total | < 16.7 ms | 60 FPS budget |

**Dirty-flag pattern**: each component calls `redraw()` only when its
state has actually changed. `draw(Canvas&)` is only invoked by Visage
when `redraw()` has been called since the last frame.

```cpp
void StepGrid::onPlayheadUpdate(int step) {
    if (step != mCurrentStep) {
        mCurrentStep = step;
        redraw();  // only when position actually changes
    }
}
```

**Never call `redraw()` unconditionally every frame** — this forces all
components to repaint every frame and destroys the frame budget.
