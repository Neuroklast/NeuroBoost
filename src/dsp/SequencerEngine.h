#pragma once

#include <cstdint>
#include <random>

#include "../common/Constants.h"
#include "../common/Types.h"
#include "../common/Presets.h"
#include "Algorithms.h"
#include "NoteTracker.h"
#include "ParamSmoother.h"
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

  // ---- State access for serialization (UI thread only) ----
  const SequenceStep* getSteps() const { return mSteps; }
  void setStep(int index, const SequenceStep& step);

  char getLSystemAxiom() const { return mLSystemAxiom; }
  const char* getLSystemRuleA() const { return mLSystemRuleA; }
  const char* getLSystemRuleB() const { return mLSystemRuleB; }
  int getLSystemIterations() const { return mLSystemIterations; }

  int getCARule() const { return mCARule; }
  int getCAIterations() const { return mCAIterations; }

  double getFractalCx() const { return mFractalCx; }
  double getFractalCy() const { return mFractalCy; }
  double getFractalZoom() const { return mFractalZoom; }
  int getFractalMaxIter() const { return mFractalMaxIter; }
  int getFractalThreshold() const { return mFractalThreshold; }

  const double (*getMarkovMatrix() const)[12] { return mMarkovMatrix; }
  int getMarkovStartNote() const { return mMarkovStartNote; }
  void setMarkovMatrix(const double matrix[12][12]);
  void setMarkovStartNote(int note) { mMarkovStartNote = note; }

  // ---- Swing ----
  void setSwing(double swing);

  // ---- Mutation rate ----
  void setMutationRate(double rate);

  // ---- Velocity curve ----
  void setVelocityCurve(double curve);

  // ---- Octave range ----
  void setOctaveRange(int low, int high);

  // ---- Per-step editing ----
  void setStepPitch(int index, int pitch);
  void setStepVelocity(int index, double velocity);
  void setStepProbability(int index, double probability);
  void setStepDuration(int index, double durationBeats);
  void setStepRatchet(int index, int count, double decay);
  void setStepCondition(int index, ConditionMode mode, int param);
  void setStepMicroTiming(int index, double offset);
  void setStepActive(int index, bool active);
  void setStepAccent(int index, bool accent);

  // ---- Panic ----

  /// Force all active notes off immediately.
  /// Writes up to MAX_POLYPHONY notes to notesOut[] and returns the count.
  /// Realtime-safe (no heap, no mutex).
  int panicAllNotes(ActiveNote* notesOut) { return mNoteTracker.panic(notesOut); }

  // ---- Preset loading ----
  void loadPreset(const PresetData& preset);

private:
  // Fire notes for a single sequencer step using the given smoothed parameters.
  void generateStepNotes(int stepIndex, double stepBeat, int nFrames,
                         double density, double swing,
                         double velocityCurve, double mutationRate);

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

  // Swing, mutation, velocity curve, octave range
  double         mSwing;
  double         mMutationRate;
  double         mVelocityCurve;
  int            mOctaveLow;
  int            mOctaveHigh;

  // Cycle counter (incremented each time mCurrentStep wraps to 0)
  int            mCycleCount;

  // Output buffers
  MidiNote       mOutputNotes[MAX_POLYPHONY];
  int            mOutputNoteCount;

  MidiNote       mNoteOffNotes[MAX_POLYPHONY];
  int            mNoteOffCount;

  NoteTracker    mNoteTracker;
  TransportSync  mTransport;
  std::mt19937   mRng;

  // ---- Parameter smoothers (realtime-safe) ----
  ParamSmoother  mDensitySmoother;
  ParamSmoother  mSwingSmoother;
  ParamSmoother  mVelocityCurveSmoother;
  ParamSmoother  mMutationRateSmoother;
};
