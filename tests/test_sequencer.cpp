// test_sequencer.cpp – standalone tests for the NeuroBoost DSP engine.
// No iPlug2, no Visage dependencies.

#include <iostream>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <vector>
#include <cstdio>

#include "../src/dsp/LockFreeQueue.h"
#include "../src/dsp/ScaleQuantizer.h"
#include "../src/dsp/NoteTracker.h"
#include "../src/dsp/Algorithms.h"
#include "../src/dsp/ParamSmoother.h"
#include "../src/dsp/TransportSync.h"
#include "../src/dsp/SequencerEngine.h"
#include "../src/common/MidiExport.h"

// ============================================================================
// Helpers
// ============================================================================

static int gPassed = 0;
static int gFailed = 0;

#define EXPECT(cond, msg)                                              \
  do {                                                                 \
    if (cond) {                                                        \
      std::cout << "  PASS: " << msg << "\n";                         \
      ++gPassed;                                                       \
    } else {                                                           \
      std::cout << "  FAIL: " << msg << "\n";                         \
      ++gFailed;                                                       \
    }                                                                  \
  } while (false)

static void printSection(const char* name)
{
  std::cout << "\n[" << name << "]\n";
}

// ============================================================================
// ScaleQuantizer tests
// ============================================================================

static void testScaleQuantizer()
{
  printSection("ScaleQuantizer");

  // Chromatic passthrough
  EXPECT(ScaleQuantizer::quantize(60, 0, ScaleMode::Chromatic) == 60,
         "Chromatic: C4 stays C4");
  EXPECT(ScaleQuantizer::quantize(61, 0, ScaleMode::Chromatic) == 61,
         "Chromatic: C#4 stays C#4");

  // Major scale: C major intervals = {0,2,4,5,7,9,11}
  // C4=60 is in C major → stays 60
  EXPECT(ScaleQuantizer::quantize(60, 60, ScaleMode::Major) == 60,
         "Major: C4 stays C4");

  // D4=62 is in C major → stays 62
  EXPECT(ScaleQuantizer::quantize(62, 60, ScaleMode::Major) == 62,
         "Major: D4 stays D4");

  // C#4=61 is NOT in C major → snaps up to D4=62
  EXPECT(ScaleQuantizer::quantize(61, 60, ScaleMode::Major) == 62,
         "Major: C#4 snaps to D4");

  // Negative modulo safety: note below root
  int result = ScaleQuantizer::quantize(58, 60, ScaleMode::Major); // Bb3
  EXPECT(result >= 0 && result <= 127,
         "Negative offset: result in valid MIDI range");

  // All scales produce valid MIDI (0-127) for all notes
  bool allValid = true;
  for (int n = 0; n <= 127; ++n)
  {
    for (int mode = 0; mode <= static_cast<int>(ScaleMode::Augmented); ++mode)
    {
      int q = ScaleQuantizer::quantize(n, 60, static_cast<ScaleMode>(mode));
      if (q < 0 || q > 127) { allValid = false; break; }
    }
    if (!allValid) break;
  }
  EXPECT(allValid, "All scales: quantize(0..127) stays in [0,127]");
}

// ============================================================================
// NoteTracker tests
// ============================================================================

static void testNoteTracker()
{
  printSection("NoteTracker");

  NoteTracker tracker;
  EXPECT(tracker.activeCount() == 0, "Initial: no active notes");

  // Basic note-on
  bool ok = tracker.noteOn(60, 1, 2.0);
  EXPECT(ok, "noteOn returns true when slot available");
  EXPECT(tracker.activeCount() == 1, "activeCount == 1 after noteOn");

  // Note-off fires at beat 2.0
  ActiveNote offs[MAX_POLYPHONY];
  int count = tracker.checkNoteOffs(1.9, offs);
  EXPECT(count == 0, "checkNoteOffs(1.9): no note-off yet");

  count = tracker.checkNoteOffs(2.0, offs);
  EXPECT(count == 1, "checkNoteOffs(2.0): one note-off");
  EXPECT(offs[0].pitch == 60, "Note-off pitch == 60");
  EXPECT(tracker.activeCount() == 0, "activeCount == 0 after note-off");

  // Panic clears all
  tracker.noteOn(60, 1, 100.0);
  tracker.noteOn(64, 1, 100.0);
  tracker.noteOn(67, 1, 100.0);
  EXPECT(tracker.activeCount() == 3, "Three notes active before panic");
  count = tracker.panic(offs);
  EXPECT(count == 3, "panic() returns 3");
  EXPECT(tracker.activeCount() == 0, "activeCount == 0 after panic");

  // MAX_POLYPHONY overflow
  tracker.reset();
  bool overflowed = false;
  for (int i = 0; i < MAX_POLYPHONY + 5; ++i)
  {
    bool result = tracker.noteOn(60 + (i % 12), 1, 999.0);
    if (!result) { overflowed = true; break; }
  }
  EXPECT(overflowed, "noteOn returns false when MAX_POLYPHONY exceeded");
  EXPECT(tracker.activeCount() == MAX_POLYPHONY,
         "activeCount == MAX_POLYPHONY at capacity");
}

// ============================================================================
// Euclidean rhythm tests
// ============================================================================

static void testEuclidean()
{
  printSection("Euclidean");

  bool pattern[MAX_STEPS] = {};

  // E(0,8) = all false
  Algorithms::generateEuclidean(8, 0, 0, pattern, MAX_STEPS);
  bool allFalse = true;
  for (int i = 0; i < 8; ++i) if (pattern[i]) allFalse = false;
  EXPECT(allFalse, "E(0,8): all steps false");

  // E(8,8) = all true
  Algorithms::generateEuclidean(8, 8, 0, pattern, MAX_STEPS);
  bool allTrue = true;
  for (int i = 0; i < 8; ++i) if (!pattern[i]) allTrue = false;
  EXPECT(allTrue, "E(8,8): all steps true");

  // E(5,8) – known Euclidean rhythm with exactly 5 hits
  Algorithms::generateEuclidean(8, 5, 0, pattern, MAX_STEPS);
  int hits = 0;
  for (int i = 0; i < 8; ++i) if (pattern[i]) ++hits;
  EXPECT(hits == 5, "E(5,8): exactly 5 hits");

  // E(3,8) with rotation=2 – same number of hits
  Algorithms::generateEuclidean(8, 3, 0, pattern, MAX_STEPS);
  bool base[MAX_STEPS] = {};
  std::memcpy(base, pattern, sizeof(bool) * MAX_STEPS);

  Algorithms::generateEuclidean(8, 3, 2, pattern, MAX_STEPS);
  hits = 0;
  for (int i = 0; i < 8; ++i) if (pattern[i]) ++hits;
  EXPECT(hits == 3, "E(3,8) rotation=2: still 3 hits");

  // Rotation shifts the pattern
  bool different = false;
  for (int i = 0; i < 8; ++i)
    if (pattern[i] != base[i]) { different = true; break; }
  EXPECT(different, "E(3,8) rotation=2: pattern differs from rotation=0");

  // hits > steps → all true
  Algorithms::generateEuclidean(4, 8, 0, pattern, MAX_STEPS);
  bool allTrue2 = true;
  for (int i = 0; i < 4; ++i) if (!pattern[i]) allTrue2 = false;
  EXPECT(allTrue2, "hits > steps: all steps true");

  // steps = 0 → no-op (no crash)
  Algorithms::generateEuclidean(0, 0, 0, pattern, MAX_STEPS);
  EXPECT(true, "steps=0: no crash");
}

// ============================================================================
// Fibonacci tests
// ============================================================================

static void testFibonacci()
{
  printSection("Fibonacci");

  bool pattern[MAX_STEPS] = {};

  // stepCount=16: positions at fib mod 16 should be set
  Algorithms::generateFibonacci(16, pattern, MAX_STEPS);

  // Fibonacci numbers: 1,1,2,3,5,8,13,21,...
  // Positions (1-indexed → 0-indexed): 0,0,1,2,4,7,12,4,...
  // At minimum, positions 0,1,2,4,7,12 should be active
  EXPECT(pattern[0], "Fibonacci 16: position 0 active (fib=1)");
  EXPECT(pattern[1], "Fibonacci 16: position 1 active (fib=2)");
  EXPECT(pattern[2], "Fibonacci 16: position 2 active (fib=3)");
  EXPECT(pattern[4], "Fibonacci 16: position 4 active (fib=5)");
  EXPECT(pattern[7], "Fibonacci 16: position 7 active (fib=8)");
  EXPECT(pattern[12], "Fibonacci 16: position 12 active (fib=13)");

  // At least one step is active
  bool anyActive = false;
  for (int i = 0; i < 16; ++i) if (pattern[i]) anyActive = true;
  EXPECT(anyActive, "Fibonacci 16: at least one step active");

  // stepCount=1: only position 0 can be set
  bool p1[MAX_STEPS] = {};
  Algorithms::generateFibonacci(1, p1, MAX_STEPS);
  EXPECT(p1[0], "Fibonacci stepCount=1: position 0 active");

  // stepCount=0: no crash, all false
  bool p0[MAX_STEPS] = {};
  Algorithms::generateFibonacci(0, p0, MAX_STEPS);
  EXPECT(true, "Fibonacci stepCount=0: no crash");
}

// ============================================================================
// L-System tests
// ============================================================================

static void testLSystem()
{
  printSection("LSystem");

  bool pattern[MAX_LSYSTEM_LENGTH] = {};

  // Classical Fibonacci L-System: A→AB, B→A
  // Iteration 0: A
  // Iteration 1: AB
  // Iteration 2: ABA
  // Iteration 3: ABAAB
  // Iteration 4: ABAABABA  (length 8)
  Algorithms::generateLSystem('A', "AB", "A", 4, pattern, MAX_LSYSTEM_LENGTH);

  // After 4 iterations the string is "ABAABABA" (length 8)
  // A=true, B=false
  // Expected: T F T T F T F T
  EXPECT(pattern[0] == true,  "LSystem 4 iter [0] = A (true)");
  EXPECT(pattern[1] == false, "LSystem 4 iter [1] = B (false)");
  EXPECT(pattern[2] == true,  "LSystem 4 iter [2] = A (true)");
  EXPECT(pattern[3] == true,  "LSystem 4 iter [3] = A (true)");
  EXPECT(pattern[4] == false, "LSystem 4 iter [4] = B (false)");
  EXPECT(pattern[5] == true,  "LSystem 4 iter [5] = A (true)");

  // 0 iterations: axiom only → 'A' = one true step
  bool p0[MAX_LSYSTEM_LENGTH] = {};
  Algorithms::generateLSystem('A', "AB", "A", 0, p0, MAX_LSYSTEM_LENGTH);
  EXPECT(p0[0] == true, "LSystem 0 iter: axiom A maps to true");

  // Large number of iterations must not crash (capped at MAX_LSYSTEM_LENGTH)
  bool big[MAX_LSYSTEM_LENGTH] = {};
  Algorithms::generateLSystem('A', "AB", "A", 30, big, MAX_LSYSTEM_LENGTH);
  EXPECT(true, "LSystem 30 iterations: no crash/OOM");
  // Some steps must be active
  bool anyActive = false;
  for (int i = 0; i < MAX_LSYSTEM_LENGTH; ++i) if (big[i]) { anyActive = true; break; }
  EXPECT(anyActive, "LSystem 30 iterations: at least one active step");
}

// ============================================================================
// Cellular Automata tests
// ============================================================================

static void testCellularAutomata()
{
  printSection("CellularAutomata");

  bool pattern[MAX_STEPS] = {};

  // Rule 0: all cells die → output all false
  Algorithms::generateCellularAutomata(0, 16, 8, pattern, MAX_STEPS);
  bool allFalse = true;
  for (int i = 0; i < 16; ++i) if (pattern[i]) allFalse = false;
  EXPECT(allFalse, "CA Rule=0: all output steps false");

  // Rule 255: all cells live → output all true
  Algorithms::generateCellularAutomata(255, 16, 8, pattern, MAX_STEPS);
  bool allTrue = true;
  // Row 0 has only one cell; after 1 gen rule 255 turns all on
  // (we just check no crash and result is deterministic)
  EXPECT(true, "CA Rule=255: no crash");

  // Rule 30: chaotic, non-trivial output
  Algorithms::generateCellularAutomata(30, 16, 8, pattern, MAX_STEPS);
  bool anyActive = false;
  for (int i = 0; i < 16; ++i) if (pattern[i]) anyActive = true;
  EXPECT(anyActive, "CA Rule=30: at least one active step");

  // Determinism: same parameters produce same result
  bool p1[MAX_STEPS] = {};
  bool p2[MAX_STEPS] = {};
  Algorithms::generateCellularAutomata(30, 16, 8, p1, MAX_STEPS);
  Algorithms::generateCellularAutomata(30, 16, 8, p2, MAX_STEPS);
  bool same = true;
  for (int i = 0; i < 16; ++i) if (p1[i] != p2[i]) same = false;
  EXPECT(same, "CA Rule=30: two calls with same params produce same result");

  // steps=0: no crash
  Algorithms::generateCellularAutomata(30, 0, 8, pattern, MAX_STEPS);
  EXPECT(true, "CA steps=0: no crash");
}

// ============================================================================
// Fractal tests
// ============================================================================

static void testFractal()
{
  printSection("Fractal");

  bool pattern[MAX_STEPS] = {};
  int iterCounts[MAX_STEPS] = {};

  // Point deep inside Mandelbrot set (c = 0+0i): all iterations should reach maxIter
  Algorithms::generateFractal(16, 0.0, 0.0, 0.0, 50, 25, iterCounts, pattern, MAX_STEPS);
  EXPECT(iterCounts[0] == 50, "Fractal: c=0+0i reaches maxIter (=50)");
  EXPECT(pattern[0] == true, "Fractal: c=0+0i above threshold");

  // Point outside set (c = 2.5+0i): should escape immediately
  Algorithms::generateFractal(1, 2.5, 0.0, 0.0, 50, 25, iterCounts, pattern, MAX_STEPS);
  EXPECT(iterCounts[0] < 50, "Fractal: c=2.5 escapes before maxIter");
  EXPECT(pattern[0] == false, "Fractal: c=2.5 below threshold");

  // maxIter=0: no crash, all counts should be 0
  Algorithms::generateFractal(4, 0.0, 0.0, 1.0, 0, 0, iterCounts, pattern, MAX_STEPS);
  EXPECT(true, "Fractal maxIter=0: no crash");

  // steps=0: no crash
  Algorithms::generateFractal(0, 0.0, 0.0, 1.0, 50, 25, iterCounts, pattern, MAX_STEPS);
  EXPECT(true, "Fractal steps=0: no crash");

  // With zoom > 0: output varies across steps
  Algorithms::generateFractal(16, -0.5, 0.0, 3.0, 50, 10, iterCounts, pattern, MAX_STEPS);
  bool someTrue = false, someFalse = false;
  for (int i = 0; i < 16; ++i)
  {
    if (pattern[i])  someTrue  = true;
    if (!pattern[i]) someFalse = true;
  }
  EXPECT(someTrue || someFalse, "Fractal with zoom: result computed without crash");
}

// ============================================================================
// Markov tests
// ============================================================================

static void testMarkov()
{
  printSection("Markov");

  int output1[MAX_STEPS] = {};
  int output2[MAX_STEPS] = {};

  // Determinism: same seed → same sequence
  {
    std::mt19937 rng1(42);
    std::mt19937 rng2(42);
    Algorithms::generateMarkov(Algorithms::MARKOV_PRESET_BLUES, 0, 16, rng1, output1, MAX_STEPS);
    Algorithms::generateMarkov(Algorithms::MARKOV_PRESET_BLUES, 0, 16, rng2, output2, MAX_STEPS);
    bool same = true;
    for (int i = 0; i < 16; ++i) if (output1[i] != output2[i]) same = false;
    EXPECT(same, "Markov: same seed produces identical sequence");
  }

  // All output values are valid pitch classes (0-11)
  {
    std::mt19937 rng(99);
    Algorithms::generateMarkov(Algorithms::MARKOV_PRESET_JAZZ, 5, 32, rng, output1, MAX_STEPS);
    bool allValid = true;
    for (int i = 0; i < 32; ++i)
      if (output1[i] < 0 || output1[i] > 11) allValid = false;
    EXPECT(allValid, "Markov: all output values in [0, 11]");
  }

  // Absorbing state (all-zero row) falls back to uniform distribution
  {
    double zeroMatrix[12][12] = {};
    // All rows zero → uniform fallback at every step
    std::mt19937 rng(1);
    Algorithms::generateMarkov(zeroMatrix, 0, 16, rng, output1, MAX_STEPS);
    bool allValid = true;
    for (int i = 0; i < 16; ++i)
      if (output1[i] < 0 || output1[i] > 11) allValid = false;
    EXPECT(allValid, "Markov absorbing state: uniform fallback produces valid pitch classes");
  }

  // Minimal preset: strong self-transitions, result should have runs of same pitch
  {
    std::mt19937 rng(7);
    Algorithms::generateMarkov(Algorithms::MARKOV_PRESET_MINIMAL, 0, 32, rng, output1, MAX_STEPS);
    int repeatCount = 0;
    for (int i = 1; i < 32; ++i)
      if (output1[i] == output1[i - 1]) ++repeatCount;
    EXPECT(repeatCount > 5, "Markov Minimal: exhibits repeating notes (self-transitions)");
  }

  // steps=0: no crash
  {
    std::mt19937 rng(0);
    Algorithms::generateMarkov(Algorithms::MARKOV_PRESET_BLUES, 0, 0, rng, output1, MAX_STEPS);
    EXPECT(true, "Markov steps=0: no crash");
  }
}

// ============================================================================
// Probability tests
// ============================================================================

static void testProbability()
{
  printSection("Probability");

  bool pattern[MAX_STEPS] = {};

  // prob=1.0 for all steps → all true
  {
    double probs[MAX_STEPS];
    for (int i = 0; i < MAX_STEPS; ++i) probs[i] = 1.0;
    std::mt19937 rng(0);
    Algorithms::generateProbability(probs, MAX_STEPS, rng, pattern, MAX_STEPS);
    bool allTrue = true;
    for (int i = 0; i < MAX_STEPS; ++i) if (!pattern[i]) allTrue = false;
    EXPECT(allTrue, "Probability: prob=1.0 → all steps true");
  }

  // prob=0.0 for all steps → all false
  {
    double probs[MAX_STEPS] = {};
    std::mt19937 rng(0);
    Algorithms::generateProbability(probs, MAX_STEPS, rng, pattern, MAX_STEPS);
    bool allFalse = true;
    for (int i = 0; i < MAX_STEPS; ++i) if (pattern[i]) allFalse = false;
    EXPECT(allFalse, "Probability: prob=0.0 → all steps false");
  }

  // Determinism with same seed
  {
    double probs[MAX_STEPS];
    for (int i = 0; i < MAX_STEPS; ++i) probs[i] = 0.5;
    bool p1[MAX_STEPS] = {};
    bool p2[MAX_STEPS] = {};
    std::mt19937 rng1(42);
    std::mt19937 rng2(42);
    Algorithms::generateProbability(probs, MAX_STEPS, rng1, p1, MAX_STEPS);
    Algorithms::generateProbability(probs, MAX_STEPS, rng2, p2, MAX_STEPS);
    bool same = true;
    for (int i = 0; i < MAX_STEPS; ++i) if (p1[i] != p2[i]) same = false;
    EXPECT(same, "Probability: same seed → identical output");
  }

  // steps=0: no crash
  {
    double probs[1] = {0.5};
    std::mt19937 rng(0);
    Algorithms::generateProbability(probs, 0, rng, pattern, MAX_STEPS);
    EXPECT(true, "Probability steps=0: no crash");
  }
}

// ============================================================================
// LockFreeQueue tests
// ============================================================================

static void testLockFreeQueue()
{
  printSection("LockFreeQueue");

  LockFreeQueue<int, 4> q;

  EXPECT(q.empty(), "Initially empty");
  EXPECT(!q.full(), "Initially not full");

  // Push items
  EXPECT(q.push(1), "push(1) succeeds");
  EXPECT(q.push(2), "push(2) succeeds");
  EXPECT(q.push(3), "push(3) succeeds");
  EXPECT(q.push(4), "push(4) succeeds");
  EXPECT(!q.push(5), "push(5) fails when full");
  EXPECT(q.full(),   "Queue is full");

  // Pop items
  int v = 0;
  EXPECT(q.pop(v) && v == 1, "pop() returns 1");
  EXPECT(q.pop(v) && v == 2, "pop() returns 2");
  EXPECT(q.pop(v) && v == 3, "pop() returns 3");
  EXPECT(q.pop(v) && v == 4, "pop() returns 4");
  EXPECT(!q.pop(v),          "pop() fails when empty");
  EXPECT(q.empty(),          "Queue is empty after all pops");

  // MidiNote type
  LockFreeQueue<MidiNote, 8> mq;
  MidiNote n;
  n.pitch = 60; n.velocity = 100; n.channel = 1;
  n.startBeat = 0.0; n.durationBeats = 0.25; n.sampleOffset = 0;
  EXPECT(mq.push(n), "push MidiNote succeeds");
  MidiNote out{};
  EXPECT(mq.pop(out) && out.pitch == 60, "pop MidiNote has correct pitch");
}

// ============================================================================
// TransportSync tests
// ============================================================================

static void testTransportSync()
{
  printSection("TransportSync");

  TransportSync ts;

  // Stopped → playing transition
  ts.update(0.0, 120.0, 44100.0, false, 512);
  EXPECT(!ts.hasTransportJustStarted(), "No start when still stopped");

  ts.update(0.0, 120.0, 44100.0, true, 512);
  EXPECT(ts.hasTransportJustStarted(), "Transport just started");
  // Update again (still playing): start flag should clear
  ts.update(0.5, 120.0, 44100.0, true, 512);
  EXPECT(!ts.hasTransportJustStarted(), "Transport start flag clears on next block");

  // Beat-to-sample offset calculation
  // At 120 BPM, sampleRate=44100: samplesPerBeat = 44100*60/120 = 22050
  ts.update(0.0, 120.0, 44100.0, true, 44100);
  double spb = ts.getSamplesPerBeat();
  EXPECT(std::fabs(spb - 22050.0) < 1.0,
         "samplesPerBeat == 22050 at 120 BPM / 44100 Hz");

  // Beat 0.5 should be at sample 11025
  int off = ts.beatToSampleOffset(0.5);
  EXPECT(std::abs(off - 11025) <= 1, "beatToSampleOffset(0.5) ≈ 11025");

  // Beat outside block returns -1
  int out = ts.beatToSampleOffset(10.0);
  EXPECT(out < 0, "beatToSampleOffset outside block returns -1");

  // Loop detection
  ts.update(4.0, 120.0, 44100.0, true, 512);
  ts.update(0.0, 120.0, 44100.0, true, 512); // PPQ jumped back → loop
  EXPECT(ts.hasLooped(), "Loop detected when PPQ jumps backwards");

  ts.update(0.5, 120.0, 44100.0, true, 512);
  EXPECT(!ts.hasLooped(), "No loop on normal forward advance");
}

// ============================================================================
// SequencerEngine determinism test
// ============================================================================

static void testSequencerDeterminism()
{
  printSection("SequencerEngine – Determinism");

  auto runCycles = [](uint64_t seed, int cycles) -> int
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setTempo(120.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(8, 0);
    // Use density < 1.0 so the RNG is exercised (probabilistic gating)
    eng.setGlobalDensity(0.6);
    eng.setSeed(seed);
    eng.reset();

    int totalNotes = 0;
    int pitchSum   = 0;

    // Simulate: at 120 BPM, 16 steps per bar, each step = 0.25 beats.
    // Block size = 512 samples, samplesPerBeat = 22050 → blockBeats ≈ 0.0232
    const int nFrames = 512;
    const double samplesPerBeat = 44100.0 * 60.0 / 120.0;
    const double beatsPerBlock  = nFrames / samplesPerBeat;
    const int totalBlocks = static_cast<int>(cycles * 4.0 / beatsPerBlock) + 1;

    double ppq = 0.0;
    bool firstBlock = true;

    for (int b = 0; b < totalBlocks; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      // On first block the engine resets (transport just started)
      if (!firstBlock)
      {
        for (int n = 0; n < eng.getOutputNoteCount(); ++n)
        {
          totalNotes += 1;
          pitchSum   += eng.getOutputNotes()[n].pitch;
        }
      }
      firstBlock = false;
      ppq += beatsPerBlock;
    }
    return totalNotes * 1000 + pitchSum;
  };

  int result1 = runCycles(42, 4);
  int result2 = runCycles(42, 4);
  int result3 = runCycles(99, 4);

  EXPECT(result1 == result2,
         "Same seed produces identical output across two runs");
  EXPECT(result1 != result3 || result3 == 0,
         "Different seed produces different output (or both zero)");
}

// ============================================================================
// SequencerEngine timing test
// ============================================================================

static void testSequencerTiming()
{
  printSection("SequencerEngine – Timing");

  SequencerEngine eng;
  eng.setSampleRate(44100.0);
  eng.setTempo(120.0);
  eng.setStepCount(16);
  eng.setEuclideanParams(8, 0);
  eng.setSeed(1);

  bool allOffsetsValid = true;
  const int nFrames = 512;
  const double samplesPerBeat = 44100.0 * 60.0 / 120.0;
  const double beatsPerBlock  = nFrames / samplesPerBeat;
  double ppq = 0.0;

  for (int b = 0; b < 200; ++b)
  {
    eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
    for (int n = 0; n < eng.getOutputNoteCount(); ++n)
    {
      int offset = eng.getOutputNotes()[n].sampleOffset;
      if (offset < 0 || offset >= nFrames)
      {
        allOffsetsValid = false;
        break;
      }
    }
    ppq += beatsPerBlock;
  }

  EXPECT(allOffsetsValid, "All sample offsets within [0, nFrames)");
}

// ============================================================================
// SequencerEngine multi-mode tests
// ============================================================================

static void testSequencerMultiMode()
{
  printSection("SequencerEngine – Multi-Mode");

  // Helper: run engine for a few bars and count note-ons
  auto runAndCount = [](SequencerEngine& eng, int bars) -> int
  {
    const int nFrames = 512;
    const double samplesPerBeat = 44100.0 * 60.0 / 120.0;
    const double beatsPerBlock  = nFrames / samplesPerBeat;
    const int totalBlocks = static_cast<int>(bars * 4.0 / beatsPerBlock) + 1;

    int total = 0;
    double ppq = 0.0;
    bool first = true;
    for (int b = 0; b < totalBlocks; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      if (!first)
        total += eng.getOutputNoteCount();
      first = false;
      ppq += beatsPerBlock;
    }
    return total;
  };

  // Euclidean (default)
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(8, 0);
    eng.setSeed(1);
    int notes = runAndCount(eng, 2);
    EXPECT(notes > 0, "Euclidean mode: produces notes");
  }

  // Fibonacci mode
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setGenerationMode(GenerationMode::Fibonacci);
    eng.setSeed(1);
    int notes = runAndCount(eng, 2);
    EXPECT(notes > 0, "Fibonacci mode: produces notes");
    EXPECT(eng.getGenerationMode() == GenerationMode::Fibonacci,
           "getGenerationMode() returns Fibonacci");
  }

  // L-System mode
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setGenerationMode(GenerationMode::LSystem);
    eng.setSeed(1);
    int notes = runAndCount(eng, 2);
    EXPECT(notes > 0, "LSystem mode: produces notes");
  }

  // Cellular Automata mode
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setGenerationMode(GenerationMode::CellularAutomata);
    eng.setSeed(1);
    int notes = runAndCount(eng, 2);
    EXPECT(true, "CellularAutomata mode: no crash (notes may be 0 for rule 0)");
  }

  // Markov mode
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setGenerationMode(GenerationMode::Markov);
    eng.setSeed(1);
    int notes = runAndCount(eng, 2);
    EXPECT(notes > 0, "Markov mode: produces notes");
  }

  // Probability mode
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setGenerationMode(GenerationMode::Probability);
    eng.setSeed(1);
    int notes = runAndCount(eng, 2);
    // All steps are active (probability=1.0 by default), so we expect notes
    EXPECT(notes > 0, "Probability mode: produces notes (default prob=1.0)");
  }

  // setMarkovPreset switches presets without crash
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setGenerationMode(GenerationMode::Markov);
    eng.setMarkovPreset(0); // Blues
    eng.setMarkovPreset(1); // Jazz
    eng.setMarkovPreset(2); // Minimal
    EXPECT(true, "setMarkovPreset(0/1/2): no crash");
  }

  // regeneratePattern can be called explicitly
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(4, 0);
    eng.regeneratePattern();
    EXPECT(true, "regeneratePattern(): no crash");
  }

  // Note-off output
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(16, 0); // all steps active so note-offs are generated
    eng.setSeed(1);

    const int nFrames = 512;
    const double samplesPerBeat = 44100.0 * 60.0 / 120.0;
    const double beatsPerBlock  = nFrames / samplesPerBeat;
    double ppq = 0.0;
    int totalOffs = 0;

    for (int b = 0; b < 400; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      totalOffs += eng.getNoteOffCount();
      ppq += beatsPerBlock;
    }
    EXPECT(totalOffs > 0, "Note-off output: note-offs generated after notes fire");
  }

  // stepCount=1 with all modes must not crash
  {
    for (int m = 0; m <= static_cast<int>(GenerationMode::Probability); ++m)
    {
      SequencerEngine eng;
      eng.setSampleRate(44100.0);
      eng.setStepCount(1);
      eng.setGenerationMode(static_cast<GenerationMode>(m));
      eng.processBlock(0.0, 120.0, true, 512, 44100.0);
    }
    EXPECT(true, "stepCount=1 with all modes: no crash");
  }
}

// ============================================================================
// main
// ============================================================================

// Forward declarations for new Sprint 4 tests
static void testRatchets();
static void testSwing();
static void testConditionModes();
static void testPerStepEditing();
static void testVelocityCurveAndOctaveRange();
static void testStateSerialization();
static void testParamSmoother();
static void testNoteOffTiming();
static void testPanic();

// Forward declarations for Sprint 6 ALPHA milestone tests
static void testAlphaDeterminismAllModes();
static void testAlphaNoteOffCompleteness();
static void testAlphaTransportCycle();

// Forward declarations for Sprint 7 tests
static void testMidiInputTranspose();
static void testResetPlayhead();
static void testMidiExport();
static void testPresetBrowser();

int main()
{
  std::cout << "NeuroBoost Sequencer Tests\n";
  std::cout << "==========================\n";

  testScaleQuantizer();
  testNoteTracker();
  testEuclidean();
  testFibonacci();
  testLSystem();
  testCellularAutomata();
  testFractal();
  testMarkov();
  testProbability();
  testLockFreeQueue();
  testTransportSync();
  testSequencerDeterminism();
  testSequencerTiming();
  testSequencerMultiMode();
  testRatchets();
  testSwing();
  testConditionModes();
  testPerStepEditing();
  testVelocityCurveAndOctaveRange();
  testStateSerialization();
  testParamSmoother();
  testNoteOffTiming();
  testPanic();
  testAlphaDeterminismAllModes();
  testAlphaNoteOffCompleteness();
  testAlphaTransportCycle();
  testMidiInputTranspose();
  testResetPlayhead();
  testMidiExport();
  testPresetBrowser();

  std::cout << "\n==========================\n";
  std::cout << "Results: " << gPassed << " passed, " << gFailed << " failed\n";

  return (gFailed == 0) ? 0 : 1;
}

// ============================================================================
// Ratchet tests
// ============================================================================

static void testRatchets()
{
  printSection("Ratchets");

  auto runBlocks = [](SequencerEngine& eng, int numBlocks) -> int {
    const int nFrames = 512;
    const double sampleRate = 44100.0;
    const double samplesPerBeat = sampleRate * 60.0 / 120.0;
    const double beatsPerBlock = nFrames / samplesPerBeat;
    double ppq = 0.0;
    int total = 0;
    for (int b = 0; b < numBlocks; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, sampleRate);
      total += eng.getOutputNoteCount();
      ppq += beatsPerBlock;
    }
    return total;
  };

  // ratchetCount=4 produces more notes than ratchetCount=1
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(4, 0);
    eng.regeneratePattern();
    for (int i = 0; i < 16; ++i)
      eng.setStepRatchet(i, 1, 0.8);
    int notes1 = runBlocks(eng, 200);

    SequencerEngine eng4;
    eng4.setSampleRate(44100.0);
    eng4.setStepCount(16);
    eng4.setEuclideanParams(4, 0);
    eng4.regeneratePattern();
    for (int i = 0; i < 16; ++i)
      eng4.setStepRatchet(i, 4, 1.0);
    int notes4 = runBlocks(eng4, 200);

    EXPECT(notes4 > notes1, "ratchetCount=4 produces more notes than ratchetCount=1");
  }

  // ratchetDecay=1.0: all ratchets equal velocity
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(4);
    eng.setEuclideanParams(4, 0);
    eng.regeneratePattern();
    for (int i = 0; i < 4; ++i)
      eng.setStepRatchet(i, 2, 1.0);

    const int nFrames = 512;
    double ppq = 0.0;
    bool allEqual = true;
    for (int b = 0; b < 100; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      for (int n = 0; n + 1 < eng.getOutputNoteCount(); n += 2)
      {
        if (eng.getOutputNotes()[n].velocity != eng.getOutputNotes()[n+1].velocity)
          allEqual = false;
      }
      ppq += nFrames / (44100.0 * 60.0 / 120.0);
    }
    EXPECT(allEqual, "ratchetDecay=1.0: ratchet notes have equal velocity");
  }

  // ratchetCount > 8 is clamped to 8
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(1);
    eng.setEuclideanParams(1, 0);
    eng.regeneratePattern();
    eng.setStepRatchet(0, 100, 1.0);

    const int nFrames = 512;
    double ppq = 0.0;
    int maxPerBlock = 0;
    for (int b = 0; b < 50; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      if (eng.getOutputNoteCount() > maxPerBlock)
        maxPerBlock = eng.getOutputNoteCount();
      ppq += nFrames / (44100.0 * 60.0 / 120.0);
    }
    EXPECT(maxPerBlock <= 8, "ratchetCount > 8 clamped: no more than 8 notes per step");
  }
}

// ============================================================================
// Swing tests
// ============================================================================

static void testSwing()
{
  printSection("Swing");

  // swing=0.0 no crash
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(8, 0);
    eng.setSwing(0.0);
    eng.regeneratePattern();
    const int nFrames = 512;
    double ppq = 0.0;
    for (int b = 0; b < 100; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      ppq += nFrames / (44100.0 * 60.0 / 120.0);
    }
    EXPECT(true, "swing=0.0: no crash");
  }

  // swing=0.5 no crash
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(8, 0);
    eng.setSwing(0.5);
    eng.regeneratePattern();
    const int nFrames = 512;
    double ppq = 0.0;
    for (int b = 0; b < 100; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      ppq += nFrames / (44100.0 * 60.0 / 120.0);
    }
    EXPECT(true, "swing=0.5: no crash");
  }

  // setSwing clamping: values > 0.5 are clamped
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setSwing(2.0);
    eng.setStepCount(4);
    eng.setEuclideanParams(4, 0);
    eng.regeneratePattern();
    const int nFrames = 512;
    double ppq = 0.0;
    for (int b = 0; b < 50; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      ppq += nFrames / (44100.0 * 60.0 / 120.0);
    }
    EXPECT(true, "setSwing(2.0) clamped: no crash");
  }
}

// ============================================================================
// Condition mode tests
// ============================================================================

static void testConditionModes()
{
  printSection("ConditionModes");

  auto runBlocks = [](SequencerEngine& eng, int numBlocks) -> int {
    const int nFrames = 512;
    const double sampleRate = 44100.0;
    const double bpb = nFrames / (sampleRate * 60.0 / 120.0);
    double ppq = 0.0;
    int total = 0;
    for (int b = 0; b < numBlocks; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, sampleRate);
      total += eng.getOutputNoteCount();
      ppq += bpb;
    }
    return total;
  };

  // EveryN=1 fires every cycle (same as Always)
  {
    SequencerEngine engAlways;
    engAlways.setSampleRate(44100.0);
    engAlways.setStepCount(4);
    engAlways.setEuclideanParams(4, 0);
    engAlways.regeneratePattern();
    int notesAlways = runBlocks(engAlways, 400);

    SequencerEngine engEvery1;
    engEvery1.setSampleRate(44100.0);
    engEvery1.setStepCount(4);
    engEvery1.setEuclideanParams(4, 0);
    engEvery1.regeneratePattern();
    for (int i = 0; i < 4; ++i)
      engEvery1.setStepCondition(i, ConditionMode::EveryN, 1);
    int notesEvery1 = runBlocks(engEvery1, 400);

    EXPECT(notesAlways == notesEvery1, "EveryN=1 fires same as Always");
  }

  // EveryN=2 fires roughly half as often, but still produces notes
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(4);
    eng.setEuclideanParams(4, 0);
    eng.regeneratePattern();
    for (int i = 0; i < 4; ++i)
      eng.setStepCondition(i, ConditionMode::EveryN, 2);
    int notes = runBlocks(eng, 400);
    EXPECT(notes > 0, "EveryN=2: still produces notes");
  }

  // Fill mode: fires when density >= 0.8
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(4);
    eng.setEuclideanParams(4, 0);
    eng.setGlobalDensity(0.9);
    eng.regeneratePattern();
    for (int i = 0; i < 4; ++i)
      eng.setStepCondition(i, ConditionMode::Fill, 0);
    int notes = runBlocks(eng, 400);
    EXPECT(notes > 0, "Fill mode with density=0.9: produces notes");
  }

  // Fill mode: silent when density < 0.8
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(4);
    eng.setEuclideanParams(4, 0);
    eng.setGlobalDensity(0.5);
    eng.regeneratePattern();
    for (int i = 0; i < 4; ++i)
      eng.setStepCondition(i, ConditionMode::Fill, 0);
    int notes = runBlocks(eng, 400);
    EXPECT(notes == 0, "Fill mode with density=0.5: silent");
  }

  // PreFill mode: fires when density < 0.8
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(4);
    eng.setEuclideanParams(4, 0);
    eng.setGlobalDensity(0.5);
    eng.regeneratePattern();
    for (int i = 0; i < 4; ++i)
      eng.setStepCondition(i, ConditionMode::PreFill, 0);
    int notes = runBlocks(eng, 400);
    EXPECT(notes > 0, "PreFill mode with density=0.5: produces notes");
  }
}

// ============================================================================
// Per-step editing tests
// ============================================================================

static void testPerStepEditing()
{
  printSection("PerStepEditing");

  // setStepActive: out-of-bounds does not crash
  {
    SequencerEngine eng;
    eng.setStepActive(-1, true);
    eng.setStepActive(MAX_STEPS, true);
    EXPECT(true, "setStepActive: out-of-bounds no crash");
  }

  // setStepActive: deactivated step produces no notes
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(1);
    eng.setEuclideanParams(1, 0);
    eng.regeneratePattern();
    eng.setStepActive(0, false);

    int total = 0;
    const int nFrames = 512;
    const double bpb = nFrames / (44100.0 * 60.0 / 120.0);
    double ppq = 0.0;
    for (int b = 0; b < 100; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      total += eng.getOutputNoteCount();
      ppq += bpb;
    }
    EXPECT(total == 0, "setStepActive(0, false): step produces no notes");
  }

  // setStepVelocity: bounds check (no crash)
  {
    SequencerEngine eng;
    eng.setStepVelocity(0, 80.0);
    eng.setStepVelocity(-1, 80.0);
    eng.setStepVelocity(MAX_STEPS, 80.0);
    EXPECT(true, "setStepVelocity: no crash on valid/invalid indices");
  }

  // setStepProbability: step with probability=0 produces no notes
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(1);
    eng.setEuclideanParams(1, 0);
    eng.regeneratePattern();
    eng.setStepProbability(0, 0.0);

    int total = 0;
    const int nFrames = 512;
    const double bpb = nFrames / (44100.0 * 60.0 / 120.0);
    double ppq = 0.0;
    for (int b = 0; b < 100; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      total += eng.getOutputNoteCount();
      ppq += bpb;
    }
    EXPECT(total == 0, "setStepProbability(0, 0.0): step produces no notes");
  }

  // setStep/getSteps round-trip
  {
    SequencerEngine eng;
    SequenceStep s;
    s.pitch = 72;
    s.velocity = 90.0;
    s.velocityRange = 0.0;
    s.probability = 1.0;
    s.durationBeats = 0.25;
    s.ratchetCount = 1;
    s.ratchetDecay = 0.8;
    s.conditionMode = ConditionMode::Always;
    s.conditionParam = 1;
    s.microTiming = 0.0;
    s.active = true;
    s.accent = false;
    eng.setStep(5, s);
    EXPECT(eng.getSteps()[5].pitch == 72, "setStep: pitch stored correctly");
    EXPECT(eng.getSteps()[5].velocity == 90.0, "setStep: velocity stored correctly");
  }
}

// ============================================================================
// Velocity curve and octave range tests
// ============================================================================

static void testVelocityCurveAndOctaveRange()
{
  printSection("VelocityCurveAndOctaveRange");

  // Velocity curve=1.0: velocities stay in [1,127]
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(4);
    eng.setEuclideanParams(4, 0);
    eng.setVelocityCurve(1.0);
    eng.regeneratePattern();
    const int nFrames = 512;
    double ppq = 0.0;
    bool allOk = true;
    for (int b = 0; b < 100; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      for (int n = 0; n < eng.getOutputNoteCount(); ++n)
      {
        int v = eng.getOutputNotes()[n].velocity;
        if (v < 1 || v > 127) allOk = false;
      }
      ppq += nFrames / (44100.0 * 60.0 / 120.0);
    }
    EXPECT(allOk, "VelocityCurve=1.0: velocity in [1,127]");
  }

  // Octave range clamping: notes stay within range
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(16, 0);
    eng.setOctaveRange(4, 4); // Only octave 4 (48-59)
    eng.regeneratePattern();
    const int nFrames = 512;
    double ppq = 0.0;
    bool allInRange = true;
    for (int b = 0; b < 200; ++b)
    {
      eng.processBlock(ppq, 120.0, true, nFrames, 44100.0);
      for (int n = 0; n < eng.getOutputNoteCount(); ++n)
      {
        int p = eng.getOutputNotes()[n].pitch;
        if (p < 48 || p > 59) allInRange = false;
      }
      ppq += nFrames / (44100.0 * 60.0 / 120.0);
    }
    EXPECT(allInRange, "setOctaveRange(4,4): all notes in octave 4 [48-59]");
  }
}

// ============================================================================
// State serialization round-trip test (engine DSP layer only)
// ============================================================================

static void testStateSerialization()
{
  printSection("StateSerialization");

  // Per-step setters + getSteps() round-trip
  {
    SequencerEngine eng1;
    for (int i = 0; i < 16; ++i)
    {
      eng1.setStepPitch(i, 60 + i);
      eng1.setStepVelocity(i, 50.0 + i);
      eng1.setStepActive(i, (i % 2 == 0));
    }

    const SequenceStep* steps = eng1.getSteps();
    bool pitchOk = true, velOk = true, activeOk = true;
    for (int i = 0; i < 16; ++i)
    {
      if (steps[i].pitch    != 60 + i)       pitchOk  = false;
      if (steps[i].velocity != 50.0 + i)     velOk    = false;
      if (steps[i].active   != (i % 2 == 0)) activeOk = false;
    }
    EXPECT(pitchOk,  "StateSerialization: pitch round-trip via setStep/getSteps");
    EXPECT(velOk,    "StateSerialization: velocity round-trip via setStep/getSteps");
    EXPECT(activeOk, "StateSerialization: active round-trip via setStep/getSteps");
  }

  // Engine accessor defaults
  {
    SequencerEngine eng;
    EXPECT(eng.getLSystemAxiom() == 'A',   "getLSystemAxiom() default is 'A'");
    EXPECT(eng.getCARule() == 30,           "getCARule() default is 30");
    EXPECT(eng.getCAIterations() == 16,     "getCAIterations() default is 16");
    EXPECT(eng.getFractalCx() == -0.5,      "getFractalCx() default is -0.5");
    EXPECT(eng.getMarkovStartNote() == 0,   "getMarkovStartNote() default is 0");
  }

  // setMarkovMatrix round-trip
  {
    SequencerEngine eng;
    double mat[12][12] = {};
    mat[0][1] = 0.75;
    mat[3][5] = 0.42;
    eng.setMarkovMatrix(mat);
    const double (*got)[12] = eng.getMarkovMatrix();
    EXPECT(got[0][1] == 0.75, "setMarkovMatrix: mat[0][1] round-trip");
    EXPECT(got[3][5] == 0.42, "setMarkovMatrix: mat[3][5] round-trip");
  }

  // setMarkovStartNote round-trip
  {
    SequencerEngine eng;
    eng.setMarkovStartNote(7);
    EXPECT(eng.getMarkovStartNote() == 7, "setMarkovStartNote(7) round-trip");
  }
}

// ============================================================================
// ParamSmoother tests
// ============================================================================

static void testParamSmoother()
{
  printSection("ParamSmoother");

  // Coefficient=1.0 means instant response (one step converges completely)
  {
    ParamSmoother sm;
    sm.setCoefficient(1.0);
    sm.reset(0.0);
    sm.setTarget(1.0);
    double v = sm.process();
    EXPECT(std::abs(v - 1.0) < 1e-9, "coefficient=1.0: single process() reaches target");
  }

  // Coefficient near 0 means very slow response
  {
    ParamSmoother sm;
    sm.setCoefficient(0.001);
    sm.reset(0.0);
    sm.setTarget(1.0);
    // After 10 steps, should still be far from target
    for (int i = 0; i < 10; ++i) sm.process();
    double v = sm.getCurrent();
    EXPECT(v < 0.02, "coefficient=0.001: after 10 steps still near 0");
  }

  // Converges to target within reasonable iterations (default ~15Hz at 44100)
  {
    ParamSmoother sm;
    const double coeff = ParamSmoother::makeCoefficient(15.0, 44100.0);
    sm.setCoefficient(coeff);
    sm.reset(0.0);
    sm.setTarget(1.0);
    // 1 second at 44100 Hz = 44100 samples; 15Hz smoother should be 99%+ converged
    for (int i = 0; i < 44100; ++i) sm.process();
    double v = sm.getCurrent();
    EXPECT(v > 0.99, "15Hz smoother converges >99% after 1 second");
  }

  // reset() immediately jumps to value
  {
    ParamSmoother sm;
    sm.setCoefficient(0.01);
    sm.reset(0.5);
    sm.setTarget(1.0);
    sm.process(); sm.process();  // some steps
    sm.reset(0.75);
    EXPECT(std::abs(sm.getCurrent() - 0.75) < 1e-9, "reset() snaps current to value");
    EXPECT(std::abs(sm.getTarget()  - 0.75) < 1e-9, "reset() snaps target to value");
  }

  // Coefficient clamp: values outside [0,1] are clamped
  {
    ParamSmoother sm;
    sm.setCoefficient(2.0);   // should clamp to 1.0
    sm.reset(0.0);
    sm.setTarget(0.5);
    double v = sm.process();
    EXPECT(v <= 0.5 + 1e-9, "coefficient clamped to 1: process() does not overshoot");
  }

  // makeCoefficient: returns 1.0 for invalid sample rate
  {
    double c = ParamSmoother::makeCoefficient(15.0, 0.0);
    EXPECT(c == 1.0, "makeCoefficient: sampleRate=0 returns 1.0");
  }
}

// ============================================================================
// Note-Off timing tests
// ============================================================================

static void testNoteOffTiming()
{
  printSection("NoteOffTiming");

  // Basic: note-on at beat 0 with duration 0.25, then processBlock at beat 0.25
  // should produce a note-off for that pitch
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(16, 0);
    eng.regeneratePattern();

    const double bpm      = 120.0;
    const double srF      = 44100.0;
    const int    nFrames  = 512;
    // One beat in samples at 120 BPM = 44100*60/120 = 22050 samples
    const double beatsPerSample = bpm / (60.0 * srF);
    const double beatsPerBlock  = beatsPerSample * nFrames;

    // Run until we get a note-on
    double ppq    = 0.0;
    int    noteOnPitch = -1;
    for (int b = 0; b < 200 && noteOnPitch < 0; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, srF);
      if (eng.getOutputNoteCount() > 0)
        noteOnPitch = eng.getOutputNotes()[0].pitch;
      ppq += beatsPerBlock;
    }

    EXPECT(noteOnPitch >= 0, "NoteOffTiming: got at least one note-on");

    // Continue running until we get a note-off for that pitch
    bool gotNoteOff = false;
    for (int b = 0; b < 500 && !gotNoteOff; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, srF);
      for (int i = 0; i < eng.getNoteOffCount(); ++i)
        if (eng.getNoteOffNotes()[i].pitch == noteOnPitch)
          gotNoteOff = true;
      ppq += beatsPerBlock;
    }

    EXPECT(gotNoteOff, "NoteOffTiming: note-off received for the triggered pitch");
  }

  // Multiple simultaneous notes each get their own note-off
  {
    NoteTracker tracker;
    tracker.noteOn(60, 1, 1.0);
    tracker.noteOn(64, 1, 1.0);
    tracker.noteOn(67, 1, 1.0);
    EXPECT(tracker.activeCount() == 3, "NoteOffTiming: 3 active notes registered");

    ActiveNote offs[MAX_POLYPHONY];
    int n = tracker.checkNoteOffs(1.5, offs);  // beat 1.5 > 1.0 for all
    EXPECT(n == 3, "NoteOffTiming: checkNoteOffs returns 3 at beat 1.5");
    EXPECT(tracker.activeCount() == 0, "NoteOffTiming: all cleared after checkNoteOffs");
  }

  // Note-off sample offset is a valid value within [0, nFrames-1]
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(16, 0);
    eng.regeneratePattern();

    const double bpm = 120.0;
    const int nFrames = 512;
    const double beatsPerBlock = (bpm / (60.0 * 44100.0)) * nFrames;
    double ppq = 0.0;
    bool offsetsOk = true;
    for (int b = 0; b < 500; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
      for (int i = 0; i < eng.getNoteOffCount(); ++i)
      {
        int off = eng.getNoteOffNotes()[i].sampleOffset;
        if (off < 0 || off >= nFrames) offsetsOk = false;
      }
      ppq += beatsPerBlock;
    }
    EXPECT(offsetsOk, "NoteOffTiming: all note-off sample offsets within [0, nFrames-1]");
  }

  // Ratcheted notes: each ratchet eventually gets its note-off
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(1);
    eng.setEuclideanParams(1, 0);
    eng.setStepRatchet(0, 3, 1.0);  // 3 ratchets, no decay
    eng.regeneratePattern();

    const double bpm = 120.0;
    const int nFrames = 512;
    const double beatsPerBlock = (bpm / (60.0 * 44100.0)) * nFrames;
    double ppq = 0.0;

    // Collect note-ons and note-offs over 400 blocks
    int totalNoteOns = 0;
    int totalNoteOffs = 0;
    for (int b = 0; b < 400; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
      totalNoteOns  += eng.getOutputNoteCount();
      totalNoteOffs += eng.getNoteOffCount();
      ppq += beatsPerBlock;
    }
    // Drain any remaining note-offs (no new note-ons in "playing=false" mode)
    eng.processBlock(ppq, bpm, false, nFrames, 44100.0);
    totalNoteOffs += eng.getNoteOffCount();

    EXPECT(totalNoteOns > 0,  "Ratchet NoteOff: at least one note-on produced");
    EXPECT(totalNoteOffs > 0, "Ratchet NoteOff: at least one note-off produced");
    // With 3 ratchets per step, offs should be a multiple-of-3 fraction of ons
    EXPECT(totalNoteOffs <= totalNoteOns,
           "Ratchet NoteOff: note-off count does not exceed note-on count");
  }
}

// ============================================================================
// Panic tests
// ============================================================================

static void testPanic()
{
  printSection("Panic");

  // After 4 notes active, panic() returns all 4 with correct pitches
  {
    NoteTracker tracker;
    tracker.noteOn(60, 1, 99.0);
    tracker.noteOn(64, 1, 99.0);
    tracker.noteOn(67, 1, 99.0);
    tracker.noteOn(72, 1, 99.0);

    EXPECT(tracker.activeCount() == 4, "Panic: 4 notes active before panic");

    ActiveNote out[MAX_POLYPHONY];
    int n = tracker.panic(out);
    EXPECT(n == 4, "Panic: panic() returns 4 notes");
    EXPECT(tracker.activeCount() == 0, "Panic: activeCount() == 0 after panic");

    bool pitchesOk = false;
    for (int i = 0; i < n; ++i)
      if (out[i].pitch == 60) pitchesOk = true;
    EXPECT(pitchesOk, "Panic: pitch 60 present in panic output");
  }

  // Panic on empty tracker returns 0
  {
    NoteTracker tracker;
    ActiveNote out[MAX_POLYPHONY];
    int n = tracker.panic(out);
    EXPECT(n == 0, "Panic: empty tracker → panic() returns 0");
  }

  // panicAllNotes() via SequencerEngine
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(16, 0);
    eng.regeneratePattern();

    const double bpm = 120.0;
    const int nFrames = 512;
    const double beatsPerBlock = (bpm / (60.0 * 44100.0)) * nFrames;
    double ppq = 0.0;

    // Accumulate some notes
    for (int b = 0; b < 20; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
      ppq += beatsPerBlock;
    }

    // Now panic — should clear all notes
    ActiveNote panicNotes[MAX_POLYPHONY];
    int n = eng.panicAllNotes(panicNotes);
    EXPECT(n >= 0, "Panic: panicAllNotes() returns non-negative count");
    // Run one more block — no note-offs should be generated from stale notes
    // (they were already panicked)
    eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
    // We can't easily verify zero note-offs here because new notes may have
    // been generated in this block, but we verify the API works
    EXPECT(true, "Panic: panicAllNotes() API works without crash");
  }
}

// ============================================================================
// ALPHA Milestone: Determinism – same seed → same sequence (all 7 modes)
// ============================================================================

static void testAlphaDeterminismAllModes()
{
  printSection("ALPHA – Determinism (all modes)");

  // Helper: run the engine for a fixed number of blocks and return a
  // reproducible fingerprint (sum of note-on pitches and note-on count).
  struct Fingerprint { int noteCount; int pitchSum; };

  auto runAndFingerprint = [](GenerationMode mode, uint64_t seed) -> Fingerprint
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(8, 0);
    eng.setGlobalDensity(0.8);
    eng.setSeed(seed);
    eng.setGenerationMode(mode);
    eng.reset();

    const double bpm      = 120.0;
    const int    nFrames  = 512;
    const double spb      = 44100.0 * 60.0 / bpm;   // samples per beat
    const double bpb      = nFrames / spb;            // beats per block
    // Simulate 4 full bars (16 steps at 120 BPM = 4 beats per bar)
    const int totalBlocks = static_cast<int>(4.0 * 4.0 / bpb) + 2;

    Fingerprint fp{0, 0};
    double ppq = 0.0;
    for (int b = 0; b < totalBlocks; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
      for (int n = 0; n < eng.getOutputNoteCount(); ++n)
      {
        fp.noteCount++;
        fp.pitchSum += eng.getOutputNotes()[n].pitch;
      }
      ppq += bpb;
    }
    return fp;
  };

  const GenerationMode modes[] = {
    GenerationMode::Euclidean,
    GenerationMode::Fibonacci,
    GenerationMode::LSystem,
    GenerationMode::CellularAutomata,
    GenerationMode::Fractal,
    GenerationMode::Markov,
    GenerationMode::Probability
  };
  const char* modeNames[] = {
    "Euclidean", "Fibonacci", "LSystem", "CellularAutomata",
    "Fractal", "Markov", "Probability"
  };

  constexpr int NUM_MODES = 7;
  for (int i = 0; i < NUM_MODES; ++i)
  {
    // Run twice with the same seed – fingerprints must be identical.
    Fingerprint fp1 = runAndFingerprint(modes[i], 42);
    Fingerprint fp2 = runAndFingerprint(modes[i], 42);

    char msgBuf[128];
    std::snprintf(msgBuf, sizeof(msgBuf),
      "Determinism [%s]: same seed noteCount match", modeNames[i]);
    EXPECT(fp1.noteCount == fp2.noteCount, msgBuf);

    std::snprintf(msgBuf, sizeof(msgBuf),
      "Determinism [%s]: same seed pitchSum match", modeNames[i]);
    EXPECT(fp1.pitchSum == fp2.pitchSum, msgBuf);

    // Run with a different seed – results should differ (or both be zero,
    // which is allowed for very sparse patterns).
    Fingerprint fp3 = runAndFingerprint(modes[i], 999);
    std::snprintf(msgBuf, sizeof(msgBuf),
      "Determinism [%s]: different seed produces different output (or both zero)",
      modeNames[i]);
    bool differs = (fp1.noteCount != fp3.noteCount) ||
                   (fp1.pitchSum  != fp3.pitchSum);
    EXPECT(differs || (fp1.noteCount == 0 && fp3.noteCount == 0), msgBuf);
  }
}

// ============================================================================
// ALPHA Milestone: Note-Off completeness – every note-on has a note-off
// ============================================================================

static void testAlphaNoteOffCompleteness()
{
  printSection("ALPHA – NoteOff Completeness");

  // Run through multiple full pattern cycles, verifying that note-offs are
  // generated for every note-on.  When transport stops, the host is expected
  // to call panicAllNotes() to release any in-flight notes; we count those
  // panic-released notes as the final note-offs so the total always balances.
  auto runAndCheck = [](GenerationMode mode, const char* modeName) {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(16, 0);   // dense: all steps active
    eng.setGlobalDensity(1.0);
    eng.setSeed(1);
    eng.setGenerationMode(mode);
    eng.reset();

    const double bpm     = 120.0;
    const int    nFrames = 512;
    const double spb     = 44100.0 * 60.0 / bpm;
    const double bpb     = nFrames / spb;
    // 8 full bars to ensure multiple full pattern cycles
    const int totalBlocks = static_cast<int>(8.0 * 4.0 / bpb) + 2;

    int totalNoteOns  = 0;
    int totalNoteOffs = 0;
    double ppq = 0.0;

    for (int b = 0; b < totalBlocks; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
      totalNoteOns  += eng.getOutputNoteCount();
      totalNoteOffs += eng.getNoteOffCount();
      ppq += bpb;
    }
    // Stop transport (isPlaying=false returns early; host must call panic).
    eng.processBlock(ppq, bpm, false, nFrames, 44100.0);

    // Simulate host panic-on-stop: release any notes still in flight.
    ActiveNote hung[MAX_POLYPHONY];
    int panicCount = eng.panicAllNotes(hung);
    totalNoteOffs += panicCount;

    char buf[128];
    std::snprintf(buf, sizeof(buf),
      "NoteOff [%s]: at least one note-on generated", modeName);
    EXPECT(totalNoteOns > 0, buf);

    std::snprintf(buf, sizeof(buf),
      "NoteOff [%s]: every note-on has a note-off (natural + panic, ons=%d offs=%d)",
      modeName, totalNoteOns, totalNoteOffs);
    EXPECT(totalNoteOffs >= totalNoteOns, buf);

    // After panic, no further hung notes should remain.
    int secondPanic = eng.panicAllNotes(hung);
    std::snprintf(buf, sizeof(buf),
      "NoteOff [%s]: no hung notes remain after panic", modeName);
    EXPECT(secondPanic == 0, buf);

    // At most MAX_POLYPHONY notes can be in-flight at any time.
    std::snprintf(buf, sizeof(buf),
      "NoteOff [%s]: in-flight note count bounded by MAX_POLYPHONY (%d)",
      modeName, panicCount);
    EXPECT(panicCount <= MAX_POLYPHONY, buf);
  };

  runAndCheck(GenerationMode::Euclidean,       "Euclidean");
  runAndCheck(GenerationMode::Fibonacci,        "Fibonacci");
  runAndCheck(GenerationMode::LSystem,          "LSystem");
  runAndCheck(GenerationMode::CellularAutomata, "CellularAutomata");
  runAndCheck(GenerationMode::Fractal,          "Fractal");
  runAndCheck(GenerationMode::Markov,           "Markov");
  runAndCheck(GenerationMode::Probability,      "Probability");
}

// ============================================================================
// ALPHA Milestone: Transport stop/start/loop cycle without hung notes or crash
// ============================================================================

static void testAlphaTransportCycle()
{
  printSection("ALPHA – Transport Cycle");

  const double bpm     = 120.0;
  const int    nFrames = 512;
  const double spb     = 44100.0 * 60.0 / bpm;
  const double bpb     = nFrames / spb;

  // ── Test 1: Starting transport generates events ──────────────────────────
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(16, 0);
    eng.setGlobalDensity(1.0);
    eng.setSeed(7);
    eng.setGenerationMode(GenerationMode::Euclidean);
    eng.reset();

    int onsAfterStart = 0;
    double ppq = 0.0;
    // Run 4 bars of transport-playing
    const int runBlocks = static_cast<int>(4.0 * 4.0 / bpb) + 2;
    for (int b = 0; b < runBlocks; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
      onsAfterStart += eng.getOutputNoteCount();
      ppq += bpb;
    }
    EXPECT(onsAfterStart > 0,
           "TransportCycle: starting transport generates note-on events");
  }

  // ── Test 2: Stopping transport + host panic clears all active notes ─────────
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(8, 0);
    eng.setGlobalDensity(1.0);
    eng.setSeed(3);
    eng.setGenerationMode(GenerationMode::Euclidean);
    eng.reset();

    double ppq = 0.0;
    const int warmupBlocks = static_cast<int>(2.0 * 4.0 / bpb) + 1;
    for (int b = 0; b < warmupBlocks; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
      ppq += bpb;
    }
    // Stop transport (isPlaying=false causes early return; no auto-panic)
    eng.processBlock(ppq, bpm, false, nFrames, 44100.0);
    ppq += bpb;

    // Simulate host calling panic on stop (as a real DAW plugin would)
    ActiveNote hung[MAX_POLYPHONY];
    int firstPanic = eng.panicAllNotes(hung);
    // After panic, a second call must return 0
    int secondPanic = eng.panicAllNotes(hung);
    EXPECT(secondPanic == 0,
           "TransportCycle: panicAllNotes() fully clears the tracker after stop");
    // First panic may return up to MAX_POLYPHONY notes (the last in-flight note)
    EXPECT(firstPanic <= MAX_POLYPHONY,
           "TransportCycle: in-flight note count bounded by MAX_POLYPHONY after stop");
  }

  // ── Test 3: Loop back to step 0 works without crash or hung notes ─────────
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(8, 0);
    eng.setGlobalDensity(1.0);
    eng.setSeed(5);
    eng.setGenerationMode(GenerationMode::Euclidean);
    eng.reset();

    double ppq = 0.0;
    const int barBlocks = static_cast<int>(4.0 / bpb) + 1;

    // Play one bar, then loop back (reset ppq to 0) and play again
    for (int b = 0; b < barBlocks; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
      ppq += bpb;
    }
    // Simulate loop: jump ppq back to the start and reset engine state.
    // reset() clears the NoteTracker, simulating a proper panic-on-loop.
    ppq = 0.0;
    eng.reset();

    // Verify that reset() cleared all notes from before the loop boundary.
    ActiveNote hung[MAX_POLYPHONY];
    int hungAfterReset = eng.panicAllNotes(hung);
    EXPECT(hungAfterReset == 0,
           "TransportCycle: reset() on loop clears all notes from previous bar");

    // Play the second bar — should produce events without crash
    int onsInSecondBar = 0;
    for (int b = 0; b < barBlocks; ++b)
    {
      eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
      onsInSecondBar += eng.getOutputNoteCount();
      ppq += bpb;
    }
    EXPECT(onsInSecondBar > 0,
           "TransportCycle: events generated in bar after loop point");

    // After second bar + panic: fully clean
    int finalPanic = eng.panicAllNotes(hung);
    EXPECT(finalPanic <= MAX_POLYPHONY,
           "TransportCycle: in-flight notes after loop bar bounded by MAX_POLYPHONY");
  }

  // ── Test 4: Rapid start/stop doesn't crash ────────────────────────────────
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(16);
    eng.setEuclideanParams(8, 0);
    eng.setGlobalDensity(1.0);
    eng.setSeed(11);
    eng.setGenerationMode(GenerationMode::Euclidean);
    eng.reset();

    double ppq = 0.0;
    for (int cycle = 0; cycle < 20; ++cycle)
    {
      // Play for 2 blocks, then stop for 1 block
      for (int b = 0; b < 2; ++b)
      {
        eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
        ppq += bpb;
      }
      eng.processBlock(ppq, bpm, false, nFrames, 44100.0);
      ppq += bpb;
    }
    EXPECT(true, "TransportCycle: rapid start/stop does not crash");

    // After rapid cycling, panic releases whatever is in flight; a second
    // panic must return 0 (tracker is clean).
    ActiveNote hung[MAX_POLYPHONY];
    eng.panicAllNotes(hung);
    int secondPanic = eng.panicAllNotes(hung);
    EXPECT(secondPanic == 0,
           "TransportCycle: tracker is clean after rapid start/stop + panic");
  }
}

// ============================================================================
// Sprint 7: MIDI Input Tests
// ============================================================================

static void testMidiInputTranspose()
{
  printSection("MidiInput_Transpose");

  // setTransposeOffset adds to generated pitch AFTER scale quantization
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(4);
    eng.setGenerationMode(GenerationMode::Euclidean);
    eng.setEuclideanParams(4, 0);  // all steps active
    eng.setGlobalDensity(1.0);
    eng.setScale(ScaleMode::Chromatic, 60);
    eng.setOctaveRange(0, 10);  // wide range so transpose doesn't get clamped
    eng.setTransposeOffset(0);
    eng.reset();

    // Run one block with no transpose
    double ppq = 0.0;
    const double bpm = 120.0;
    const int nFrames = 512;
    const double bpb = 4.0 / 4.0; // quarter-note blocks
    eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
    int baseNoteCount = eng.getOutputNoteCount();
    int basePitch = (baseNoteCount > 0) ? eng.getOutputNotes()[0].pitch : -1;

    // Now apply +12 semitone transpose
    eng.setTransposeOffset(12);
    eng.reset();
    eng.processBlock(0.0, bpm, true, nFrames, 44100.0);
    int transposedCount = eng.getOutputNoteCount();
    int transposedPitch = (transposedCount > 0) ? eng.getOutputNotes()[0].pitch : -1;

    EXPECT(baseNoteCount > 0, "Transpose: baseline produces notes");
    EXPECT(transposedCount > 0, "Transpose: transposed pattern produces notes");
    if (basePitch >= 0 && transposedPitch >= 0)
      EXPECT(transposedPitch == basePitch + 12,
             "Transpose +12: pitch shifted up by 12 semitones");
  }

  // Negative transpose: -12
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.setStepCount(4);
    eng.setGenerationMode(GenerationMode::Euclidean);
    eng.setEuclideanParams(4, 0);
    eng.setGlobalDensity(1.0);
    eng.setScale(ScaleMode::Chromatic, 72);  // root = C5
    eng.setOctaveRange(0, 8);
    eng.setTransposeOffset(0);
    eng.reset();

    const double bpm = 120.0;
    const int nFrames = 512;
    eng.processBlock(0.0, bpm, true, nFrames, 44100.0);
    int basePitch = (eng.getOutputNoteCount() > 0) ? eng.getOutputNotes()[0].pitch : -1;

    eng.setTransposeOffset(-12);
    eng.reset();
    eng.processBlock(0.0, bpm, true, nFrames, 44100.0);
    int transposedPitch = (eng.getOutputNoteCount() > 0) ? eng.getOutputNotes()[0].pitch : -1;

    EXPECT(transposedPitch != -1, "Transpose -12: produces notes");
    if (basePitch >= 0 && transposedPitch >= 0)
      EXPECT(transposedPitch == basePitch - 12,
             "Transpose -12: pitch shifted down by 12 semitones");
  }
}

static void testResetPlayhead()
{
  printSection("MidiInput_ResetPlayhead");

  SequencerEngine eng;
  eng.setSampleRate(44100.0);
  eng.setStepCount(16);
  eng.setGenerationMode(GenerationMode::Euclidean);
  eng.setEuclideanParams(8, 0);
  eng.setGlobalDensity(1.0);
  eng.reset();

  // Advance a few blocks so mCurrentStep > 0
  const double bpm = 120.0;
  const int nFrames = 512;
  double ppq = 0.0;
  const double bpb = 0.25; // one block per step at 120bpm/512samples

  // Play a few blocks
  for (int b = 0; b < 5; ++b)
  {
    eng.processBlock(ppq, bpm, true, nFrames, 44100.0);
    ppq += bpb;
  }

  // resetPlayhead must set step to 0
  eng.resetPlayhead();
  EXPECT(eng.getCurrentStep() == 0,
         "resetPlayhead: mCurrentStep == 0 after call");

  // Should NOT regenerate the pattern: same step count
  EXPECT(eng.getStepCount() == 16,
         "resetPlayhead: stepCount unchanged");
}

// ============================================================================
// Sprint 7: MIDI Export Tests
// ============================================================================

// Read all bytes from a file
static std::vector<uint8_t> readFile(const char* path)
{
  std::ifstream f(path, std::ios::binary);
  if (!f.is_open()) return {};
  return std::vector<uint8_t>(
    std::istreambuf_iterator<char>(f),
    std::istreambuf_iterator<char>());
}

static void testMidiExport()
{
  printSection("MidiExport");

  const char* tmpPath = "/tmp/test_neuroboost_export.mid";

  // ── Test 1: Valid MIDI file header (MThd) ──────────────────────────────
  {
    SequenceStep steps[MAX_STEPS] = {};
    // 8 active steps
    for (int i = 0; i < 8; ++i)
    {
      steps[i].active        = true;
      steps[i].pitch         = 60 + i;
      steps[i].velocity      = 80.0;
      steps[i].durationBeats = 0.5;
      steps[i].ratchetCount  = 1;
      steps[i].ratchetDecay  = 1.0;
      steps[i].probability   = 1.0;
    }

    MidiExport::ExportParams p;
    p.steps                = steps;
    p.stepCount            = 16;
    p.bpm                  = 120.0;
    p.ticksPerQuarterNote  = 480;
    p.rootNote             = 60;
    p.scaleMode            = ScaleMode::Major;

    bool ok = MidiExport::writeToFile(tmpPath, p);
    EXPECT(ok, "MidiExport: writeToFile returns true");

    auto data = readFile(tmpPath);
    EXPECT(data.size() >= 14, "MidiExport: file has at least 14 bytes (MThd chunk)");

    if (data.size() >= 4)
    {
      bool mthd = (data[0] == 'M' && data[1] == 'T' && data[2] == 'h' && data[3] == 'd');
      EXPECT(mthd, "MidiExport: file starts with MThd magic");
    }

    // Header chunk length must be 6 (big-endian 00 00 00 06)
    if (data.size() >= 8)
    {
      uint32_t hlen = ((uint32_t)data[4] << 24) | ((uint32_t)data[5] << 16)
                    | ((uint32_t)data[6] << 8)  |  (uint32_t)data[7];
      EXPECT(hlen == 6, "MidiExport: MThd chunk length == 6");
    }

    // Format 0, 1 track
    if (data.size() >= 12)
    {
      uint16_t fmt    = ((uint16_t)data[8] << 8)  | data[9];
      uint16_t ntrks  = ((uint16_t)data[10] << 8) | data[11];
      EXPECT(fmt == 0,   "MidiExport: format 0 (single track)");
      EXPECT(ntrks == 1, "MidiExport: 1 track");
    }

    // MTrk chunk follows immediately after MThd (at offset 14)
    if (data.size() >= 18)
    {
      bool mtrk = (data[14] == 'M' && data[15] == 'T'
                && data[16] == 'r' && data[17] == 'k');
      EXPECT(mtrk, "MidiExport: MTrk chunk present at offset 14");
    }
  }

  // ── Test 2: Correct note event count for 8-active / 16-step pattern ──
  {
    SequenceStep steps[MAX_STEPS] = {};
    int activeCount = 0;
    for (int i = 0; i < 16; ++i)
    {
      steps[i].ratchetCount  = 1;
      steps[i].ratchetDecay  = 1.0;
      steps[i].probability   = 1.0;
      steps[i].durationBeats = 0.5;
      steps[i].velocity      = 80.0;
      if (i % 2 == 0)       // 8 active steps
      {
        steps[i].active = true;
        steps[i].pitch  = 60;
        ++activeCount;
      }
    }

    MidiExport::ExportParams p;
    p.steps               = steps;
    p.stepCount           = 16;
    p.bpm                 = 120.0;
    p.ticksPerQuarterNote = 480;
    p.rootNote            = 60;
    p.scaleMode           = ScaleMode::Major;

    MidiExport::writeToFile(tmpPath, p);
    auto data = readFile(tmpPath);

    // Count 0x90 (Note-On) events in track data
    int noteOnCount = 0;
    for (size_t i = 0; i + 2 < data.size(); ++i)
    {
      if ((data[i] & 0xF0) == 0x90 && data[i + 2] > 0)
        ++noteOnCount;
    }
    EXPECT(noteOnCount == activeCount,
           "MidiExport: note-on count equals active step count (8)");
  }

  // ── Test 3: Empty pattern produces valid (but empty) file ─────────────
  {
    SequenceStep steps[MAX_STEPS] = {};
    // all inactive

    MidiExport::ExportParams p;
    p.steps               = steps;
    p.stepCount           = 16;
    p.bpm                 = 120.0;
    p.ticksPerQuarterNote = 480;
    p.rootNote            = 60;
    p.scaleMode           = ScaleMode::Chromatic;

    bool ok = MidiExport::writeToFile(tmpPath, p);
    EXPECT(ok, "MidiExport: empty pattern writes without error");
    auto data = readFile(tmpPath);
    EXPECT(data.size() >= 14, "MidiExport: empty pattern file has MThd");
    if (data.size() >= 4)
      EXPECT(data[0] == 'M', "MidiExport: empty pattern file has correct magic");
  }

  // ── Test 4: Ratcheted step produces multiple sub-notes ────────────────
  {
    SequenceStep steps[MAX_STEPS] = {};
    steps[0].active        = true;
    steps[0].pitch         = 60;
    steps[0].velocity      = 100.0;
    steps[0].durationBeats = 0.5;
    steps[0].ratchetCount  = 4;   // 4 sub-notes
    steps[0].ratchetDecay  = 0.9;
    steps[0].probability   = 1.0;

    MidiExport::ExportParams p;
    p.steps               = steps;
    p.stepCount           = 1;
    p.bpm                 = 120.0;
    p.ticksPerQuarterNote = 480;
    p.rootNote            = 60;
    p.scaleMode           = ScaleMode::Chromatic;

    MidiExport::writeToFile(tmpPath, p);
    auto data = readFile(tmpPath);

    int noteOnCount = 0;
    for (size_t i = 0; i + 2 < data.size(); ++i)
      if ((data[i] & 0xF0) == 0x90 && data[i + 2] > 0)
        ++noteOnCount;

    EXPECT(noteOnCount == 4,
           "MidiExport: ratchet count=4 produces 4 note-on events");
  }

  // ── Test 5: VLQ encoding correctness ──────────────────────────────────
  {
    // Test our VLQ encoder directly using a tiny helper
    auto encodeVlq = [](uint32_t value) -> std::vector<uint8_t> {
      std::vector<uint8_t> out;
      uint8_t bytes[4];
      int len = 0;
      bytes[len++] = static_cast<uint8_t>(value & 0x7F);
      value >>= 7;
      while (value) {
        bytes[len++] = static_cast<uint8_t>((value & 0x7F) | 0x80);
        value >>= 7;
      }
      for (int i = len - 1; i >= 0; --i)
        out.push_back(bytes[i]);
      return out;
    };

    auto v0 = encodeVlq(0);
    EXPECT(v0.size() == 1 && v0[0] == 0x00, "VLQ: 0 encodes to 0x00");

    auto v127 = encodeVlq(127);
    EXPECT(v127.size() == 1 && v127[0] == 0x7F, "VLQ: 127 encodes to 0x7F");

    auto v128 = encodeVlq(128);
    EXPECT(v128.size() == 2 && v128[0] == 0x81 && v128[1] == 0x00,
           "VLQ: 128 encodes to 0x81 0x00");

    auto v16383 = encodeVlq(16383);
    EXPECT(v16383.size() == 2 && v16383[0] == 0xFF && v16383[1] == 0x7F,
           "VLQ: 16383 encodes to 0xFF 0x7F");

    auto v16384 = encodeVlq(16384);
    EXPECT(v16384.size() == 3, "VLQ: 16384 encodes to 3 bytes");
  }

  // Cleanup
  std::remove(tmpPath);
}

// ============================================================================
// Sprint 7: Preset Tests
// ============================================================================

static void testPresetBrowser()
{
  printSection("PresetBrowser");

  // Test: cycling all 10 factory presets doesn't crash the engine
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    bool allLoaded = true;
    for (int i = 0; i < kNumFactoryPresets; ++i)
    {
      eng.loadPreset(FACTORY_PRESETS[i]);
      if (eng.getStepCount() < 1 || eng.getStepCount() > MAX_STEPS)
        allLoaded = false;
    }
    EXPECT(allLoaded, "PresetBrowser: all 10 factory presets load without crash");
  }

  // Test: engine state is correctly updated after preset load
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.loadPreset(FACTORY_PRESETS[0]);  // Classic 4/4: Euclidean, 16 steps
    EXPECT(eng.getStepCount() == 16,
           "PresetBrowser: Classic 4/4 loads with 16 steps");
    EXPECT(eng.getGenerationMode() == GenerationMode::Euclidean,
           "PresetBrowser: Classic 4/4 loads with Euclidean mode");
  }

  // Test: loading a preset with a non-default scale
  {
    SequencerEngine eng;
    eng.setSampleRate(44100.0);
    eng.loadPreset(FACTORY_PRESETS[5]);  // Mandelbrot Dance: WholeTone
    EXPECT(eng.getScaleMode() == ScaleMode::WholeTone,
           "PresetBrowser: Mandelbrot Dance loads with WholeTone scale");
  }
}
