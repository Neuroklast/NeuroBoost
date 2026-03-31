// SequencerEngine.cpp – implementation of the core MIDI sequencer.
// Realtime-safe: no heap allocations, no mutex, no I/O, no exceptions.

#include "SequencerEngine.h"

#include <cstring>

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

SequencerEngine::SequencerEngine()
  : mSampleRate(44100.0)
  , mTempo(DEFAULT_TEMPO)
  , mStepCount(DEFAULT_STEP_COUNT)
  , mCurrentStep(0)
  , mCurrentBeat(0.0)
  , mScaleMode(ScaleMode::Major)
  , mRootNote(DEFAULT_ROOT_NOTE)
  , mEuclideanHits(4)
  , mEuclideanRotation(0)
  , mGlobalDensity(1.0)
  , mSeed(12345)
  , mGenMode(GenerationMode::Euclidean)
  , mMarkovStartNote(0)
  , mFractalCx(-0.5)
  , mFractalCy(0.0)
  , mFractalZoom(3.0)
  , mFractalMaxIter(50)
  , mFractalThreshold(10)
  , mLSystemAxiom('A')
  , mLSystemIterations(4)
  , mCARule(30)
  , mCAIterations(16)
  , mOutputNoteCount(0)
  , mNoteOffCount(0)
  , mRng(12345)
{
  for (int i = 0; i < MAX_STEPS; ++i)
  {
    mSteps[i].pitch          = DEFAULT_ROOT_NOTE;
    mSteps[i].velocity       = DEFAULT_VELOCITY;
    mSteps[i].velocityRange  = 0.0;
    mSteps[i].probability    = DEFAULT_PROBABILITY;
    mSteps[i].durationBeats  = DEFAULT_DURATION_BEATS;
    mSteps[i].ratchetCount   = 1;
    mSteps[i].ratchetDecay   = 0.8;
    mSteps[i].conditionMode  = ConditionMode::Always;
    mSteps[i].conditionParam = 1;
    mSteps[i].microTiming    = 0.0;
    mSteps[i].active         = true;
    mSteps[i].accent         = false;
    mEuclideanPattern[i]     = false;
    mMarkovPitches[i]        = 0;
    mFractalIterCounts[i]    = 0;
  }

  // Default L-System rules: A→AB, B→A (Fibonacci word)
  for (int i = 0; i < MAX_LSYSTEM_LENGTH - 1; ++i)
    mLSystemRuleA[i] = mLSystemRuleB[i] = '\0';
  mLSystemRuleA[0] = 'A'; mLSystemRuleA[1] = 'B'; mLSystemRuleA[2] = '\0';
  mLSystemRuleB[0] = 'A'; mLSystemRuleB[1] = '\0';

  // Default Markov matrix: Blues preset
  for (int r = 0; r < 12; ++r)
    for (int c = 0; c < 12; ++c)
      mMarkovMatrix[r][c] = Algorithms::MARKOV_PRESET_BLUES[r][c];

  // Generate initial Euclidean pattern
  Algorithms::generateEuclidean(mStepCount, mEuclideanHits, mEuclideanRotation,
                                mEuclideanPattern, MAX_STEPS);
}

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

void SequencerEngine::setSampleRate(double sr)
{
  mSampleRate = sr;
}

void SequencerEngine::setTempo(double bpm)
{
  if (bpm < MIN_TEMPO) bpm = MIN_TEMPO;
  if (bpm > MAX_TEMPO) bpm = MAX_TEMPO;
  mTempo = bpm;
}

void SequencerEngine::setStepCount(int steps)
{
  if (steps < 1)         steps = 1;
  if (steps > MAX_STEPS) steps = MAX_STEPS;
  mStepCount = steps;
  regeneratePattern();
}

void SequencerEngine::setScale(ScaleMode mode, int rootNote)
{
  mScaleMode = mode;
  mRootNote  = rootNote;
}

void SequencerEngine::setEuclideanParams(int hits, int rotation)
{
  mEuclideanHits     = hits;
  mEuclideanRotation = rotation;
  if (mGenMode == GenerationMode::Euclidean)
    regeneratePattern();
}

void SequencerEngine::setGlobalDensity(double density)
{
  if (density < 0.0) density = 0.0;
  if (density > 1.0) density = 1.0;
  mGlobalDensity = density;
}

void SequencerEngine::setSeed(uint64_t seed)
{
  mSeed = seed;
  mRng.seed(static_cast<std::mt19937::result_type>(seed));
}

void SequencerEngine::setGenerationMode(GenerationMode mode)
{
  mGenMode = mode;
  regeneratePattern();
}

void SequencerEngine::setFractalParams(double cx, double cy, double zoom,
                                        int maxIter, int threshold)
{
  mFractalCx        = cx;
  mFractalCy        = cy;
  mFractalZoom      = zoom;
  mFractalMaxIter   = maxIter;
  mFractalThreshold = threshold;
  if (mGenMode == GenerationMode::Fractal)
    regeneratePattern();
}

void SequencerEngine::setMarkovPreset(int presetIndex)
{
  const double (*src)[12] = nullptr;
  if      (presetIndex == 0) src = Algorithms::MARKOV_PRESET_BLUES;
  else if (presetIndex == 1) src = Algorithms::MARKOV_PRESET_JAZZ;
  else                       src = Algorithms::MARKOV_PRESET_MINIMAL;

  for (int r = 0; r < 12; ++r)
    for (int c = 0; c < 12; ++c)
      mMarkovMatrix[r][c] = src[r][c];

  if (mGenMode == GenerationMode::Markov)
    regeneratePattern();
}

// ----------------------------------------------------------------------------
// regeneratePattern
// ----------------------------------------------------------------------------

void SequencerEngine::regeneratePattern()
{
  switch (mGenMode)
  {
    case GenerationMode::Euclidean:
      Algorithms::generateEuclidean(mStepCount, mEuclideanHits, mEuclideanRotation,
                                    mEuclideanPattern, MAX_STEPS);
      break;

    case GenerationMode::Fibonacci:
      Algorithms::generateFibonacci(mStepCount, mEuclideanPattern, MAX_STEPS);
      break;

    case GenerationMode::LSystem:
      Algorithms::generateLSystem(mLSystemAxiom, mLSystemRuleA, mLSystemRuleB,
                                  mLSystemIterations, mEuclideanPattern, MAX_STEPS);
      break;

    case GenerationMode::CellularAutomata:
      Algorithms::generateCellularAutomata(mCARule, mStepCount, mCAIterations,
                                           mEuclideanPattern, MAX_STEPS);
      break;

    case GenerationMode::Fractal:
      Algorithms::generateFractal(mStepCount, mFractalCx, mFractalCy, mFractalZoom,
                                  mFractalMaxIter, mFractalThreshold,
                                  mFractalIterCounts, mEuclideanPattern, MAX_STEPS);
      break;

    case GenerationMode::Markov:
      // All steps active; generate pitch sequence deterministically
      for (int i = 0; i < MAX_STEPS; ++i)
        mEuclideanPattern[i] = true;
      Algorithms::generateMarkov(mMarkovMatrix, mMarkovStartNote, mStepCount,
                                 mRng, mMarkovPitches, MAX_STEPS);
      break;

    case GenerationMode::Probability:
      // All steps attempt to fire; per-step probability is in mSteps[i].probability
      for (int i = 0; i < MAX_STEPS; ++i)
        mEuclideanPattern[i] = true;
      break;
  }
}

// ----------------------------------------------------------------------------
// Reset
// ----------------------------------------------------------------------------

void SequencerEngine::reset()
{
  mCurrentStep     = 0;
  mCurrentBeat     = 0.0;
  mOutputNoteCount = 0;
  mNoteOffCount    = 0;
  mNoteTracker.reset();
  mRng.seed(static_cast<std::mt19937::result_type>(mSeed));
}

// ----------------------------------------------------------------------------
// Main process loop
// ----------------------------------------------------------------------------

double SequencerEngine::stepDuration() const
{
  return 4.0 / static_cast<double>(mStepCount);
}

void SequencerEngine::processBlock(double ppqPos, double tempo, bool isPlaying,
                                   int nFrames, double sampleRate)
{
  mOutputNoteCount = 0;
  mNoteOffCount    = 0;

  mTransport.update(ppqPos, tempo, sampleRate, isPlaying, nFrames);

  if (mTransport.hasTransportJustStarted())
    reset();

  if (!isPlaying)
    return;

  const double blockStart = mTransport.getBlockStartBeat();
  const double blockEnd   = mTransport.getBlockEndBeat();

  // Collect note-offs that expire within this block
  ActiveNote noteOffBuffer[MAX_POLYPHONY];
  int nOff = mNoteTracker.checkNoteOffs(blockEnd, noteOffBuffer);
  for (int i = 0; i < nOff && mNoteOffCount < MAX_POLYPHONY; ++i)
  {
    MidiNote& off    = mNoteOffNotes[mNoteOffCount++];
    off.pitch        = noteOffBuffer[i].pitch;
    off.velocity     = 0;
    off.channel      = noteOffBuffer[i].channel;
    off.startBeat    = blockEnd;
    off.durationBeats = 0.0;
    off.sampleOffset = 0;
  }

  // Walk through all steps that fall inside [blockStart, blockEnd)
  const double sDur = stepDuration();
  while (mCurrentBeat < blockEnd && mOutputNoteCount < MAX_POLYPHONY)
  {
    if (mCurrentBeat >= blockStart)
      generateStepNotes(mCurrentStep, mCurrentBeat, nFrames);

    mCurrentBeat += sDur;
    mCurrentStep  = (mCurrentStep + 1) % mStepCount;
  }
}

void SequencerEngine::generateStepNotes(int stepIndex, double stepBeat, int nFrames)
{
  if (!mEuclideanPattern[stepIndex])
    return;

  const SequenceStep& step = mSteps[stepIndex];
  if (!step.active)
    return;

  // Probability gate
  double effectiveProbability = step.probability * mGlobalDensity;
  if (effectiveProbability < 1.0)
  {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    if (dist(mRng) > effectiveProbability)
      return;
  }

  // Pitch: Markov mode overrides the step pitch with the generated pitch class
  int basePitch = step.pitch;
  if (mGenMode == GenerationMode::Markov)
  {
    int pitchClass = mMarkovPitches[stepIndex];
    pitchClass     = ((pitchClass % 12) + 12) % 12;
    int rootOctave = mRootNote / 12;
    basePitch      = rootOctave * 12 + pitchClass;
  }

  int pitch = ScaleQuantizer::quantize(basePitch, mRootNote, mScaleMode);

  // Velocity with optional random range
  double vel = step.velocity;
  if (step.velocityRange > 0.0)
  {
    std::uniform_real_distribution<double> vDist(-step.velocityRange * 0.5,
                                                  step.velocityRange * 0.5);
    vel += vDist(mRng);
  }
  if (vel < 1.0)   vel = 1.0;
  if (vel > 127.0) vel = 127.0;

  int sampleOffset = mTransport.beatToSampleOffset(stepBeat + step.microTiming);
  if (sampleOffset < 0)        sampleOffset = 0;
  if (sampleOffset >= nFrames) sampleOffset = nFrames - 1;

  double noteOffBeat = stepBeat + step.durationBeats;
  mNoteTracker.noteOn(pitch, 1, noteOffBeat);

  if (mOutputNoteCount < MAX_POLYPHONY)
  {
    MidiNote& note     = mOutputNotes[mOutputNoteCount++];
    note.pitch         = pitch;
    note.velocity      = static_cast<int>(vel);
    note.channel       = 1;
    note.startBeat     = stepBeat;
    note.durationBeats = step.durationBeats;
    note.sampleOffset  = sampleOffset;
  }
}

