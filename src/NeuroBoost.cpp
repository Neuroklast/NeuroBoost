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
}

void NeuroBoost::OnReset()
{
  mEngine.setSampleRate(GetSampleRate());
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
      GetParam(idx)->Set(value);
      SendParameterValueFromUI(idx, GetParam(idx)->GetNormalized());
    }
  );
  mEditorView->setBounds(0, 0, mEditor->width(), mEditor->height());
  mEditor->addChild(mEditorView.get());

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

