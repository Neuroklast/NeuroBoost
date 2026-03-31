# NeuroBoost – Market Analysis

---

## What NeuroBoost Is

NeuroBoost is a **deterministic & probabilistic MIDI sequencer plugin** for
professional DAWs (VST3/AU/CLAP). It generates rhythms and melodies using
seven mathematical algorithms:

1. **Euclidean Rhythms** – Bjorklund algorithm for perfectly even hit distribution
2. **L-Systems** – Lindenmayer string rewriting for self-similar, fractal patterns
3. **Cellular Automata** – Wolfram 1D rules (e.g., Rule 30, Rule 110) for emergent rhythms
4. **Markov Chains** – Probabilistic transition matrices for genre-appropriate progressions
5. **Fractal Mapping** – Mandelbrot/Julia set iteration counts mapped to pitch and velocity
6. **Random Walk** – Constrained stochastic movement in pitch/velocity space
7. **User Pattern** – Full manual step sequencer with per-step probability and ratcheting

All generation is **deterministic**: the same seed always produces the same output.
This makes NeuroBoost suitable for both live performance (predictable evolution)
and studio use (reproducible arrangements).

The UI is built with **Visage** (the GPU-accelerated UI library from VitalAudio),
giving NeuroBoost a real-time fractal visualizer and smooth 60 FPS animations
with near-zero CPU overhead for the interface.

---

## Unique Selling Points

| USP | Details |
|-----|---------|
| **iPlug2 + Visage — first of its kind** | No other released plugin uses this combination. Sets a precedent for GPU-rendered professional plugin UIs. |
| **7 mathematical generation modes** | Competing tools offer 1–2 modes. NeuroBoost unifies Euclidean, L-System, CA, Markov, Fractal, Random Walk, and manual in a single plugin. |
| **GPU-rendered fractal visualization** | Real-time Julia/Mandelbrot set tied to audio parameters. Both a musical tool and a generative art instrument. |
| **Determinism guarantee** | Fixed seed → reproducible output. Musicians can write a seed into project notes and recall the exact pattern years later. |
| **Sample-accurate MIDI** | MIDI events sent at the correct sample offset within each block. Not all sequencer plugins achieve this. |
| **Open architecture** | DSP layer is fully decoupled from UI, enabling headless use in test environments and potential future web/CLI tools. |

---

## Competitive Landscape

| Product | Type | Euclidean | L-System | Cell. Auto. | Markov | Fractal | GPU UI | Determinism |
|---------|------|:---------:|:--------:|:-----------:|:------:|:-------:|:------:|:-----------:|
| **NeuroBoost** | VST3/AU/CLAP | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Ableton Probability Pack | Max for Live | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Rozzer / Riffer (Audiomodern) | VST3/AU | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| HY-SEQ (HY-Plugins) | VST3/AU | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Euclidean Circles (VCV Rack) | Modular (not DAW) | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Scaler 2 (Plugin Boutique) | VST3/AU | ❌ | ❌ | ❌ | ✅ (chords) | ❌ | ❌ | ❌ |
| Sequencer 64 (various) | VST3 | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| VCV Rack (various modules) | Modular | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | Partial |

### Notes on Competitors

**Ableton Probability Pack** (Max for Live)
- Requires Ableton Live Suite + Max for Live license (~€750+ total)
- Probability-based step sequencer only; no mathematical generation
- Cannot be used in any other DAW
- No GPU visualization

**Rozzer / Riffer (Audiomodern)**
- Focuses on random rhythmic and melodic generation
- No mathematical model; purely random, not deterministic
- No Euclidean, L-System, or fractal capabilities

**HY-SEQ (HY-Plugins)**
- Has Euclidean mode, good step sequencer
- No L-System, Cellular Automata, or Markov
- No GPU visualization
- Determinism not guaranteed

**VCV Rack**
- Extremely powerful modular ecosystem
- Not a DAW plugin; requires hosting VCV Rack separately
- No integrated fractal visualization
- Steep learning curve for non-modular musicians

---

## The Gap NeuroBoost Fills

**No VST3/AU plugin currently combines:**

1. Euclidean rhythms
2. L-Systems
3. Cellular Automata
4. Markov chains
5. Fractal mapping
6. ...in a single professional DAW plugin with a GPU-accelerated UI

Musicians who want any of these capabilities today must either:
- Learn VCV Rack (modular, steep curve, not integrated in DAW)
- Pay for multiple Max for Live devices (Ableton-only)
- Write code themselves (Python/SuperCollider/Max)
- Accept simple random generation without determinism

NeuroBoost closes this gap: a **single VST3/AU/CLAP plugin** that works in any
major DAW, requires no additional software, and provides all seven generation
modes in a unified, GPU-rendered interface.

---

## Target Audience

| Segment | Why NeuroBoost |
|---------|----------------|
| Electronic music producers | Euclidean + Markov for evolving patterns that feel musical |
| Experimental / ambient composers | L-System + Fractal for self-similar, organic textures |
| Live performers | Deterministic seed system: predictable evolution, no surprises |
| Sound designers | Cellular Automata for unusual rhythmic textures |
| Developers / DSP engineers | Open DSP layer, clean iPlug2+Visage reference implementation |

---

## Pricing Strategy (Reference)

| Tier | Price | Rationale |
|------|-------|-----------|
| Launch price | €39 | Competitive with HY-Plugins; accessible entry point |
| Regular price | €59 | Below Scaler 2 (€49), above HY-SEQ (~€25), justified by 7 modes + GPU UI |
| Bundle (future) | €89 | NeuroBoost + companion tools |

---

## Summary

NeuroBoost occupies a **unique, uncontested position** in the plugin market:
the only professional DAW plugin combining deterministic mathematical music
generation (Euclidean, L-System, CA, Markov, Fractal) with a GPU-rendered
real-time visualizer, built on the modern iPlug2+Visage stack.

The combination of **mathematical rigor** (reproducible, seed-based),
**visual feedback** (GPU fractal renderer), and **DAW integration** (VST3/AU/CLAP,
sample-accurate MIDI) addresses a clear gap that no existing product fills.
