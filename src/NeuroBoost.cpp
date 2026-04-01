#include "NeuroBoost.h"
#include "IPlug_include_in_plug_src.h"
#include "visage_windowing/windowing.h"
#include "visage_utils/dimension.h"
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Include centralised theme definitions
#include "ui/theme/Theme.h"

NeuroBoost::NeuroBoost(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // Step sequencer parameters
  GetParam(kStepCount)->InitInt("Step Count", DEFAULT_STEP_COUNT, 1, MAX_STEPS);
  GetParam(kGenMode)->InitEnum("Generation Mode", 0,
    { "Euclidean", "Fibonacci", "L-System", "Cellular Automata",
      "Markov", "Fractal", "Probability" });
  GetParam(kRootNote)->InitInt("Root Note", DEFAULT_ROOT_NOTE, 0, 127);
  GetParam(kScaleMode)->InitEnum("Scale Mode", 0,
    { "Chromatic", "Major", "Minor", "Dorian", "Phrygian", "Lydian",
      "Mixolydian", "HarmonicMinor", "MelodicMinor", "PentatonicMajor",
      "PentatonicMinor", "Blues", "WholeTone", "Diminished", "Augmented" });
  GetParam(kGlobalDensity)->InitDouble("Global Density", 1.0, 0.0, 1.0, 0.01);
  GetParam(kSwing)->InitDouble("Swing", 0.0, 0.0, 1.0, 0.01);
  GetParam(kEuclideanHits)->InitInt("Euclidean Hits", 4, 0, MAX_STEPS);
  GetParam(kEuclideanRotation)->InitInt("Euclidean Rotation", 0, 0, MAX_STEPS - 1);
  GetParam(kFractalSeed)->InitInt("Fractal Seed", 12345, 0, 99999);
  GetParam(kFractalDimension)->InitDouble("Fractal Dimension", -0.5, -2.5, 1.0, 0.001);
  GetParam(kFractalDepth)->InitInt("Fractal Depth", 50, 1, MAX_FRACTAL_ITERATIONS);
  GetParam(kMutationRate)->InitDouble("Mutation Rate", 0.0, 0.0, 1.0, 0.01);
  GetParam(kVelocityCurve)->InitDouble("Velocity Curve", 1.0, 0.1, 4.0, 0.01);
  GetParam(kOctaveLow)->InitInt("Octave Low", 3, 0, 8);
  GetParam(kOctaveHigh)->InitInt("Octave High", 5, 0, 8);

  // Initialise engine defaults from parameters
  mEngine.setSampleRate(44100.0);
  mEngine.setStepCount(DEFAULT_STEP_COUNT);
  mEngine.setScale(ScaleMode::Major, DEFAULT_ROOT_NOTE);
  mEngine.setEuclideanParams(4, 0);
  mEngine.setGlobalDensity(1.0);
  mEngine.setOctaveRange(3, 5);

  // Register factory presets
  for (int i = 0; i < kNumFactoryPresets; ++i)
  {
    const PresetData& p = FACTORY_PRESETS[i];
    MakePreset(p.name,
      static_cast<double>(p.stepCount),                    // kStepCount
      static_cast<double>(static_cast<int>(p.mode)),       // kGenMode
      static_cast<double>(p.rootNote),                     // kRootNote
      static_cast<double>(static_cast<int>(p.scale)),      // kScaleMode
      p.density,                                           // kGlobalDensity
      p.swing,                                             // kSwing
      static_cast<double>(p.euclideanHits),                // kEuclideanHits
      static_cast<double>(p.euclideanRotation),            // kEuclideanRotation
      12345.0,                                             // kFractalSeed
      p.fractalCx,                                         // kFractalDimension
      static_cast<double>(p.fractalMaxIter),               // kFractalDepth
      0.0,                                                 // kMutationRate
      1.0,                                                 // kVelocityCurve
      3.0,                                                 // kOctaveLow
      5.0                                                  // kOctaveHigh
    );
  }
}

void NeuroBoost::OnReset()
{
  mEngine.setSampleRate(GetSampleRate());
}

void NeuroBoost::OnActivate(bool active)
{
  if (!active)
    sendPanicNoteOffs();
}

void NeuroBoost::sendPanicNoteOffs()
{
  ActiveNote panicNotes[MAX_POLYPHONY];
  int count = mEngine.panicAllNotes(panicNotes);
  for (int i = 0; i < count; ++i)
  {
    IMidiMsg msg;
    msg.MakeNoteOffMsg(panicNotes[i].pitch, 0, panicNotes[i].channel - 1);
    SendMidiMsg(msg);
  }
}

void NeuroBoost::OnIdle()
{
  // Poll playhead position updates from the audio thread and update UI
  int step = 0;
  while (mPlayheadQueue.pop(step))
  {
    if (mEditorView)
    {
      mEditorView->setPlayheadStep(step);
      mEditorView->triggerStepFlash(step);
    }
  }

  // Sync knob positions when the host changes parameters via automation
  if (mEditorView)
  {
    for (int i = 0; i < kNumParams; ++i)
      mEditorView->updateKnobFromHost(i, GetParam(i)->Value());
  }
}

void NeuroBoost::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    case kStepCount:
      mEngine.setStepCount(static_cast<int>(GetParam(kStepCount)->Value()));
      break;

    case kGenMode:
      mEngine.setGenerationMode(
        static_cast<GenerationMode>(static_cast<int>(GetParam(kGenMode)->Value())));
      break;

    case kRootNote:
      mEngine.setScale(mEngine.getScaleMode(),
                       static_cast<int>(GetParam(kRootNote)->Value()));
      break;

    case kScaleMode:
      mEngine.setScale(
        static_cast<ScaleMode>(static_cast<int>(GetParam(kScaleMode)->Value())),
        static_cast<int>(GetParam(kRootNote)->Value()));
      break;

    case kGlobalDensity:
      mEngine.setGlobalDensity(GetParam(kGlobalDensity)->Value());
      break;

    case kSwing:
      mEngine.setSwing(GetParam(kSwing)->Value());
      break;

    case kEuclideanHits:
      mEngine.setEuclideanParams(
        static_cast<int>(GetParam(kEuclideanHits)->Value()),
        static_cast<int>(GetParam(kEuclideanRotation)->Value()));
      break;

    case kEuclideanRotation:
      mEngine.setEuclideanParams(
        static_cast<int>(GetParam(kEuclideanHits)->Value()),
        static_cast<int>(GetParam(kEuclideanRotation)->Value()));
      break;

    case kFractalSeed:
      mEngine.setSeed(static_cast<uint64_t>(GetParam(kFractalSeed)->Value()));
      break;

    case kFractalDimension:
    case kFractalDepth:
      mEngine.setFractalParams(
        GetParam(kFractalDimension)->Value(),
        0.0,
        3.0,
        static_cast<int>(GetParam(kFractalDepth)->Value()),
        static_cast<int>(GetParam(kFractalDepth)->Value()) / 2);
      break;

    case kMutationRate:
      mEngine.setMutationRate(GetParam(kMutationRate)->Value());
      break;

    case kVelocityCurve:
      mEngine.setVelocityCurve(GetParam(kVelocityCurve)->Value());
      break;

    case kOctaveLow:
    case kOctaveHigh:
      mEngine.setOctaveRange(
        static_cast<int>(GetParam(kOctaveLow)->Value()),
        static_cast<int>(GetParam(kOctaveHigh)->Value()));
      break;

    default:
      break;
  }
}

void* NeuroBoost::OpenWindow(void* pParent)
{
  mEditor = std::make_unique<visage::ApplicationEditor>();
  visage::IBounds bounds = visage::computeWindowBounds(PLUG_WIDTH, PLUG_HEIGHT);
  mEditor->setBounds(0, 0, bounds.width(), bounds.height());

  // Build the EditorView, wiring parameter get/set through the plugin
  mEditorView = std::make_unique<EditorView>(
    [this](int idx) { return GetParam(idx)->Value(); },
    [this](int idx, double value) {
      BeginInformHostOfParamChange(idx);
      GetParam(idx)->Set(value);
      SendParameterValueFromUI(idx, GetParam(idx)->GetNormalized());
      EndInformHostOfParamChange(idx);
    }
  );
  mEditorView->setBounds(0, 0, mEditor->width(), mEditor->height());
  mEditor->addChild(mEditorView.get());

  mEditorView->setOnStepActiveChanged([this](int step, bool active) {
    mEngine.setStepActive(step, active);
  });
  mEditorView->setOnStepVelocityChanged([this](int step, double velocity) {
    mEngine.setStepVelocity(step, velocity * 127.0);
  });
  mEditorView->setOnStepProbabilityChanged([this](int step, double probability) {
    mEngine.setStepProbability(step, probability);
  });
  mEditorView->setOnStepAccentToggled([this](int step, bool accent) {
    mEngine.setStepAccent(step, accent);
  });

  mWindow = visage::createPluginWindow(mEditor->width(), mEditor->height(), pParent);
  mWindow->setFixedAspectRatio(mEditor->isFixedAspectRatio());
  mEditor->addToWindow(mWindow.get());
  mWindow->show();

  OnUIOpen();
  return mWindow.get()->nativeHandle();
}

void NeuroBoost::OnParentWindowResize(int width, int height)
{
  if (mWindow)
  {
    mWindow->setWindowSize(width, height);
    mEditor->setDimensions(width, height);

    if (mEditorView)
      mEditorView->setBounds(0, 0, width, height);
  }
}

void NeuroBoost::CloseWindow()
{
  sendPanicNoteOffs();

  if (mEditor)
  {
    mEditor->removeFromWindow();
    mEditor->removeAllChildren();
  }
  mEditorView.reset();
  mWindow.reset();
  mEditor.reset();

  OnUIClose();
  IEditorDelegate::CloseWindow();
}

void NeuroBoost::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  // Get host transport info
  ITimeInfo timeInfo;
  GetTimeInfo(timeInfo);

  // Update sequencer engine
  mEngine.processBlock(timeInfo.mPPQPos, timeInfo.mTempo,
                       timeInfo.mTransportIsPlaying, nFrames, GetSampleRate());

  // Send MIDI Note-Ons
  for (int i = 0; i < mEngine.getOutputNoteCount(); ++i)
  {
    const MidiNote& note = mEngine.getOutputNotes()[i];
    IMidiMsg msg;
    msg.MakeNoteOnMsg(note.pitch, note.velocity,
                      note.sampleOffset, note.channel - 1);
    SendMidiMsg(msg);
  }

  // Send MIDI Note-Offs
  for (int i = 0; i < mEngine.getNoteOffCount(); ++i)
  {
    const MidiNote& off = mEngine.getNoteOffNotes()[i];
    IMidiMsg msg;
    msg.MakeNoteOffMsg(off.pitch, off.sampleOffset, off.channel - 1);
    SendMidiMsg(msg);
  }

  // Push playhead step to UI queue (non-blocking; drop if full)
  mPlayheadQueue.push(mEngine.getCurrentStep());

  // Pass audio through (sequencer does not modify audio)
  const int nChans = NOutChansConnected();
  for (int c = 0; c < nChans; ++c)
    if (inputs[c] != outputs[c])
      std::memcpy(outputs[c], inputs[c], static_cast<size_t>(nFrames) * sizeof(sample));
}


// ----------------------------------------------------------------------------
// State serialization
// ----------------------------------------------------------------------------

bool NeuroBoost::SerializeState(iplug::IByteChunk& chunk) const
{
  uint8_t version = 1;
  chunk.Put(&version);

  for (int i = 0; i < kNumParams; ++i)
  {
    double val = GetParam(i)->Value();
    chunk.Put(&val);
  }

  const SequenceStep* steps = mEngine.getSteps();
  for (int i = 0; i < MAX_STEPS; ++i)
  {
    chunk.Put(&steps[i].pitch);
    chunk.Put(&steps[i].velocity);
    chunk.Put(&steps[i].velocityRange);
    chunk.Put(&steps[i].probability);
    chunk.Put(&steps[i].durationBeats);
    chunk.Put(&steps[i].ratchetCount);
    chunk.Put(&steps[i].ratchetDecay);
    int cm = static_cast<int>(steps[i].conditionMode);
    chunk.Put(&cm);
    chunk.Put(&steps[i].conditionParam);
    chunk.Put(&steps[i].microTiming);
    int active = steps[i].active ? 1 : 0;
    chunk.Put(&active);
    int accent = steps[i].accent ? 1 : 0;
    chunk.Put(&accent);
  }

  uint64_t seed = static_cast<uint64_t>(GetParam(kFractalSeed)->Value());
  chunk.Put(&seed);
  int genMode = static_cast<int>(GetParam(kGenMode)->Value());
  chunk.Put(&genMode);

  char axiom = mEngine.getLSystemAxiom();
  chunk.Put(&axiom);
  const char* ruleA = mEngine.getLSystemRuleA();
  const char* ruleB = mEngine.getLSystemRuleB();
  chunk.PutBytes(ruleA, MAX_LSYSTEM_LENGTH);
  chunk.PutBytes(ruleB, MAX_LSYSTEM_LENGTH);
  int lsysIter = mEngine.getLSystemIterations();
  chunk.Put(&lsysIter);

  int caRule = mEngine.getCARule();
  int caIter = mEngine.getCAIterations();
  chunk.Put(&caRule);
  chunk.Put(&caIter);

  double fcx   = mEngine.getFractalCx();
  double fcy   = mEngine.getFractalCy();
  double fzoom = mEngine.getFractalZoom();
  int fmaxIter = mEngine.getFractalMaxIter();
  int fthresh  = mEngine.getFractalThreshold();
  chunk.Put(&fcx);
  chunk.Put(&fcy);
  chunk.Put(&fzoom);
  chunk.Put(&fmaxIter);
  chunk.Put(&fthresh);

  const double (*mat)[12] = mEngine.getMarkovMatrix();
  for (int r = 0; r < 12; ++r)
    for (int c = 0; c < 12; ++c)
      chunk.Put(&mat[r][c]);
  int markovStart = mEngine.getMarkovStartNote();
  chunk.Put(&markovStart);

  return true;
}

int NeuroBoost::UnserializeState(const iplug::IByteChunk& chunk, int startPos)
{
  uint8_t version = 0;
  startPos = chunk.Get(&version, startPos);
  if (version != 1)
    return startPos;

  for (int i = 0; i < kNumParams; ++i)
  {
    double val = 0.0;
    startPos = chunk.Get(&val, startPos);
    GetParam(i)->Set(val);
  }

  mEngine.setStepCount(static_cast<int>(GetParam(kStepCount)->Value()));
  mEngine.setGenerationMode(
    static_cast<GenerationMode>(static_cast<int>(GetParam(kGenMode)->Value())));
  mEngine.setScale(
    static_cast<ScaleMode>(static_cast<int>(GetParam(kScaleMode)->Value())),
    static_cast<int>(GetParam(kRootNote)->Value()));
  mEngine.setGlobalDensity(GetParam(kGlobalDensity)->Value());
  mEngine.setSwing(GetParam(kSwing)->Value());
  mEngine.setEuclideanParams(
    static_cast<int>(GetParam(kEuclideanHits)->Value()),
    static_cast<int>(GetParam(kEuclideanRotation)->Value()));
  mEngine.setMutationRate(GetParam(kMutationRate)->Value());
  mEngine.setVelocityCurve(GetParam(kVelocityCurve)->Value());
  mEngine.setOctaveRange(
    static_cast<int>(GetParam(kOctaveLow)->Value()),
    static_cast<int>(GetParam(kOctaveHigh)->Value()));

  for (int i = 0; i < MAX_STEPS; ++i)
  {
    SequenceStep s;
    startPos = chunk.Get(&s.pitch,          startPos);
    startPos = chunk.Get(&s.velocity,       startPos);
    startPos = chunk.Get(&s.velocityRange,  startPos);
    startPos = chunk.Get(&s.probability,    startPos);
    startPos = chunk.Get(&s.durationBeats,  startPos);
    startPos = chunk.Get(&s.ratchetCount,   startPos);
    startPos = chunk.Get(&s.ratchetDecay,   startPos);
    int cm = 0;
    startPos = chunk.Get(&cm, startPos);
    s.conditionMode = static_cast<ConditionMode>(cm);
    startPos = chunk.Get(&s.conditionParam, startPos);
    startPos = chunk.Get(&s.microTiming,    startPos);
    int active = 0, accent = 0;
    startPos = chunk.Get(&active, startPos);
    startPos = chunk.Get(&accent, startPos);
    s.active = (active != 0);
    s.accent = (accent != 0);
    mEngine.setStep(i, s);
  }

  uint64_t seed = 0;
  startPos = chunk.Get(&seed, startPos);
  mEngine.setSeed(seed);
  int genMode = 0;
  startPos = chunk.Get(&genMode, startPos);

  char axiom = 'A';
  startPos = chunk.Get(&axiom, startPos);
  char ruleA[MAX_LSYSTEM_LENGTH], ruleB[MAX_LSYSTEM_LENGTH];
  startPos = chunk.GetBytes(ruleA, MAX_LSYSTEM_LENGTH, startPos);
  startPos = chunk.GetBytes(ruleB, MAX_LSYSTEM_LENGTH, startPos);
  int lsysIter = 4;
  startPos = chunk.Get(&lsysIter, startPos);

  int caRule = 30, caIter = 16;
  startPos = chunk.Get(&caRule, startPos);
  startPos = chunk.Get(&caIter, startPos);

  double fcx = -0.5, fcy = 0.0, fzoom = 3.0;
  int fmaxIter = 50, fthresh = 10;
  startPos = chunk.Get(&fcx,      startPos);
  startPos = chunk.Get(&fcy,      startPos);
  startPos = chunk.Get(&fzoom,    startPos);
  startPos = chunk.Get(&fmaxIter, startPos);
  startPos = chunk.Get(&fthresh,  startPos);
  mEngine.setFractalParams(fcx, fcy, fzoom, fmaxIter, fthresh);

  double mat[12][12];
  for (int r = 0; r < 12; ++r)
    for (int c = 0; c < 12; ++c)
      startPos = chunk.Get(&mat[r][c], startPos);
  mEngine.setMarkovMatrix(mat);
  int markovStart = 0;
  startPos = chunk.Get(&markovStart, startPos);
  mEngine.setMarkovStartNote(markovStart);

  mEngine.regeneratePattern();

  return startPos;
}
