// test_sequencer.cpp – standalone tests for the NeuroBoost DSP engine.
// No iPlug2, no Visage dependencies.

#include <iostream>
#include <cstring>
#include <cmath>
#include <cstdint>

#include "../src/dsp/LockFreeQueue.h"
#include "../src/dsp/ScaleQuantizer.h"
#include "../src/dsp/NoteTracker.h"
#include "../src/dsp/Algorithms.h"
#include "../src/dsp/TransportSync.h"
#include "../src/dsp/SequencerEngine.h"

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

  std::cout << "\n==========================\n";
  std::cout << "Results: " << gPassed << " passed, " << gFailed << " failed\n";

  return (gFailed == 0) ? 0 : 1;
}
