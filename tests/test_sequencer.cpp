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
// main
// ============================================================================

int main()
{
  std::cout << "NeuroBoost Sequencer Tests\n";
  std::cout << "==========================\n";

  testScaleQuantizer();
  testNoteTracker();
  testEuclidean();
  testLockFreeQueue();
  testTransportSync();
  testSequencerDeterminism();
  testSequencerTiming();

  std::cout << "\n==========================\n";
  std::cout << "Results: " << gPassed << " passed, " << gFailed << " failed\n";

  return (gFailed == 0) ? 0 : 1;
}
