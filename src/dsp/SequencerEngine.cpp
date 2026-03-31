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
  , mOutputNoteCount(0)
  , mRng(12345)
{
  // Initialise all steps with sensible defaults
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
  }

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

  // Re-generate the Euclidean pattern for the new step count
  Algorithms::generateEuclidean(mStepCount, mEuclideanHits, mEuclideanRotation,
                                mEuclideanPattern, MAX_STEPS);
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
  Algorithms::generateEuclidean(mStepCount, mEuclideanHits, mEuclideanRotation,
                                mEuclideanPattern, MAX_STEPS);
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

// ----------------------------------------------------------------------------
// Reset
// ----------------------------------------------------------------------------

void SequencerEngine::reset()
{
  mCurrentStep = 0;
  mCurrentBeat = 0.0;
  mOutputNoteCount = 0;
  mNoteTracker.reset();
  // Re-seed for determinism: same seed → same sequence
  mRng.seed(static_cast<std::mt19937::result_type>(mSeed));
}

// ----------------------------------------------------------------------------
// Main process loop
// ----------------------------------------------------------------------------

double SequencerEngine::stepDuration() const
{
  // One step = one beat / 4 (sixteenth note at default) or configurable.
  // Here a step equals one quarter note divided by (stepCount / 4).
  // Simpler: keep each step as a quarter note beat for now.
  // The sequencer plays mStepCount steps per bar (4 beats).
  return 4.0 / static_cast<double>(mStepCount);
}

void SequencerEngine::processBlock(double ppqPos, double tempo, bool isPlaying,
                                   int nFrames, double sampleRate)
{
  mOutputNoteCount = 0;

  mTransport.update(ppqPos, tempo, sampleRate, isPlaying, nFrames);

  if (mTransport.hasTransportJustStarted())
    reset();

  if (!isPlaying)
    return;

  const double blockStart = mTransport.getBlockStartBeat();
  const double blockEnd   = mTransport.getBlockEndBeat();
  const double sDur       = stepDuration();

  // Emit note-offs for notes that expire within this block
  ActiveNote noteOffs[MAX_POLYPHONY];
  int nOff = mNoteTracker.checkNoteOffs(blockEnd, noteOffs);
  (void)nOff; // Note-offs are managed by NoteTracker; host handles MIDI out

  // Walk through all steps that fall inside [blockStart, blockEnd)
  while (mCurrentBeat < blockEnd && mOutputNoteCount < MAX_POLYPHONY)
  {
    if (mCurrentBeat >= blockStart)
    {
      generateStepNotes(mCurrentStep, mCurrentBeat, nFrames);
    }

    // Advance step
    mCurrentBeat += sDur;
    mCurrentStep = ((mCurrentStep + 1) % mStepCount);
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

  // Quantize pitch to scale
  int pitch = ScaleQuantizer::quantize(step.pitch, mRootNote, mScaleMode);

  // Velocity with optional random range
  double vel = step.velocity;
  if (step.velocityRange > 0.0)
  {
    std::uniform_real_distribution<double> vDist(-step.velocityRange * 0.5,
                                                  step.velocityRange * 0.5);
    vel += vDist(mRng);
  }
  if (vel < 1.0)  vel = 1.0;
  if (vel > 127.0) vel = 127.0;

  // Compute sample offset within the current block
  int sampleOffset = mTransport.beatToSampleOffset(stepBeat + step.microTiming);
  if (sampleOffset < 0)       sampleOffset = 0;
  if (sampleOffset >= nFrames) sampleOffset = nFrames - 1;

  // Register note-off
  double noteOffBeat = stepBeat + step.durationBeats;
  mNoteTracker.noteOn(pitch, 1 /* default channel */, noteOffBeat);

  // Write to output buffer
  if (mOutputNoteCount < MAX_POLYPHONY)
  {
    MidiNote& note       = mOutputNotes[mOutputNoteCount++];
    note.pitch           = pitch;
    note.velocity        = static_cast<int>(vel);
    note.channel         = 1;
    note.startBeat       = stepBeat;
    note.durationBeats   = step.durationBeats;
    note.sampleOffset    = sampleOffset;
  }
}
