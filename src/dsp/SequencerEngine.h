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

  // ---- Realtime API ----

  // Main process function. Call once per audio block.
  // Fills mOutputNotes / mOutputNoteCount with notes that start in this block.
  void processBlock(double ppqPos, double tempo, bool isPlaying,
                    int nFrames, double sampleRate);

  // Notes generated in the last processBlock() call.
  const MidiNote* getOutputNotes() const { return mOutputNotes; }
  int             getOutputNoteCount() const { return mOutputNoteCount; }

  // Index of the step currently playing (for UI playhead).
  int getCurrentStep() const { return mCurrentStep; }

  // Reset state (re-seeds RNG from stored seed, clears notes, resets step counter).
  void reset();

private:
  // Advance to the next sequencer step, returning the beat position of that step.
  // Returns -1.0 if no step fires in this block.
  void generateStepNotes(int stepIndex, double stepBeat, int nFrames);

  // Beat duration of a single step at the current step count and tempo.
  double stepDuration() const;

  // ---- State ----
  double         mSampleRate;
  double         mTempo;
  int            mStepCount;
  int            mCurrentStep;
  double         mCurrentBeat;    // beat position of the next step to fire
  ScaleMode      mScaleMode;
  int            mRootNote;
  int            mEuclideanHits;
  int            mEuclideanRotation;
  double         mGlobalDensity;
  uint64_t       mSeed;

  SequenceStep   mSteps[MAX_STEPS];
  bool           mEuclideanPattern[MAX_STEPS];
  MidiNote       mOutputNotes[MAX_POLYPHONY];
  int            mOutputNoteCount;

  NoteTracker    mNoteTracker;
  TransportSync  mTransport;
  std::mt19937   mRng;
};
