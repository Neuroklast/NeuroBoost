#pragma once

#include <cstdint>
#include <random>

#include "../common/Constants.h"
#include "../common/Types.h"
#include "Algorithms.h"
#include "NoteTracker.h"
#include "TransportSync.h"
#include "ScaleQuantizer.h"

// Core sequencer engine.
// Realtime-safe: no heap allocations, no mutex, no I/O, no exceptions.
class SequencerEngine
{
public:
  SequencerEngine();

  // ---- Configuration (may be called from any thread before processBlock) ----

  void setSampleRate(double sr);
  void setTempo(double bpm);

  // Clamps to [1, MAX_STEPS].
  void setStepCount(int steps);

  void setScale(ScaleMode mode, int rootNote);
  void setEuclideanParams(int hits, int rotation);

  // Global density scales all step probabilities (0.0 = silent, 1.0 = as-programmed).
  void setGlobalDensity(double density);

  // Store seed and re-seed the RNG.
  void setSeed(uint64_t seed);

  // Switch generation mode and regenerate the pattern immediately.
  void setGenerationMode(GenerationMode mode);
  GenerationMode getGenerationMode() const { return mGenMode; }

  // Re-run the active algorithm to refresh mEuclideanPattern and mMarkovPitches.
  void regeneratePattern();

  // Fractal parameters (call when they change; computation is cached).
  void setFractalParams(double cx, double cy, double zoom, int maxIter, int threshold);

  // Load one of the built-in Markov presets (0=Blues, 1=Jazz, 2=Minimal).
  void setMarkovPreset(int presetIndex);

  ScaleMode getScaleMode() const { return mScaleMode; }

  // ---- Realtime API ----

  // Main process function. Call once per audio block.
  // Fills mOutputNotes / mOutputNoteCount and mNoteOffNotes / mNoteOffCount.
  void processBlock(double ppqPos, double tempo, bool isPlaying,
                    int nFrames, double sampleRate);

  // Notes generated in the last processBlock() call.
  const MidiNote* getOutputNotes()    const { return mOutputNotes;    }
  int             getOutputNoteCount() const { return mOutputNoteCount; }

  // Note-offs collected in the last processBlock() call.
  const MidiNote* getNoteOffNotes()    const { return mNoteOffNotes;    }
  int             getNoteOffCount()    const { return mNoteOffCount;    }

  // Index of the step currently playing (for UI playhead).
  int getCurrentStep() const { return mCurrentStep; }

  // Reset state (re-seeds RNG from stored seed, clears notes, resets step counter).
  void reset();

private:
  // Fire notes for a single sequencer step.
  void generateStepNotes(int stepIndex, double stepBeat, int nFrames);

  // Beat duration of a single step at the current step count.
  double stepDuration() const;

  // ---- State ----
  double         mSampleRate;
  double         mTempo;
  int            mStepCount;
  int            mCurrentStep;
  double         mCurrentBeat;
  ScaleMode      mScaleMode;
  int            mRootNote;
  int            mEuclideanHits;
  int            mEuclideanRotation;
  double         mGlobalDensity;
  uint64_t       mSeed;
  GenerationMode mGenMode;

  // Per-step sequencer data
  SequenceStep   mSteps[MAX_STEPS];

  // Universal rhythm pattern (filled by regeneratePattern())
  bool           mEuclideanPattern[MAX_STEPS];

  // Markov-mode pitch sequence (pitch classes 0-11, refreshed by regeneratePattern())
  int            mMarkovPitches[MAX_STEPS];
  double         mMarkovMatrix[12][12];
  int            mMarkovStartNote;

  // Fractal-mode cached data
  int            mFractalIterCounts[MAX_STEPS];
  double         mFractalCx;
  double         mFractalCy;
  double         mFractalZoom;
  int            mFractalMaxIter;
  int            mFractalThreshold;

  // L-System parameters
  char           mLSystemAxiom;
  char           mLSystemRuleA[MAX_LSYSTEM_LENGTH];
  char           mLSystemRuleB[MAX_LSYSTEM_LENGTH];
  int            mLSystemIterations;

  // Cellular Automata parameters
  int            mCARule;
  int            mCAIterations;

  // Output buffers
  MidiNote       mOutputNotes[MAX_POLYPHONY];
  int            mOutputNoteCount;

  MidiNote       mNoteOffNotes[MAX_POLYPHONY];
  int            mNoteOffCount;

  NoteTracker    mNoteTracker;
  TransportSync  mTransport;
  std::mt19937   mRng;
};
