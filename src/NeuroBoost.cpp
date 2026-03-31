#include "NeuroBoost.h"
#include "IPlug_include_in_plug_src.h"
#include "visage_windowing/windowing.h"
#include "visage_utils/dimension.h"
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

VISAGE_THEME_COLOR(BackgroundColor, 0xff1a1a2e);
VISAGE_THEME_COLOR(KnobBackgroundColor, 0xff16213e);
VISAGE_THEME_COLOR(KnobArcColor, 0xff4a9eff);
VISAGE_THEME_COLOR(KnobTrackColor, 0xff555555);
VISAGE_THEME_COLOR(TextColor, 0xffffffff);
VISAGE_THEME_COLOR(AccentColor, 0xff00d2ff);

// GainKnob implementation (kept for Sprint 3 UI redesign)
GainKnob::GainKnob()
{
}

void GainKnob::draw(visage::Canvas& canvas)
{
  float w = width();
  float h = height();
  float size = std::min(w, h);
  float centerX = w / 2.0f;
  float centerY = h / 2.0f;
  float radius = (size - 8.0f) / 2.0f;

  canvas.setColor(KnobBackgroundColor);
  canvas.circle(centerX - radius, centerY - radius, radius * 2.0f);

  float arcDiameter = (radius - 8.0f) * 2.0f;
  float thickness = 6.0f;
  float startAngle = static_cast<float>(5.0 * M_PI / 4.0);
  float fullSweep  = static_cast<float>(3.0 * M_PI / 2.0);

  canvas.setColor(KnobTrackColor);
  canvas.flatArc(centerX - arcDiameter / 2.0f - thickness / 2.0f,
                 centerY - arcDiameter / 2.0f - thickness / 2.0f,
                 arcDiameter + thickness, thickness,
                 startAngle + fullSweep / 2.0f, fullSweep);

  float valueSweep = fullSweep * static_cast<float>(mValue);
  if (valueSweep > 0.01f)
  {
    canvas.setColor(KnobArcColor);
    canvas.flatArc(centerX - arcDiameter / 2.0f - thickness / 2.0f,
                   centerY - arcDiameter / 2.0f - thickness / 2.0f,
                   arcDiameter + thickness, thickness,
                   startAngle + valueSweep / 2.0f, valueSweep);
  }

  float indicatorAngle = startAngle + valueSweep;
  float dotRadius = 4.0f;
  float dotDistance = radius - 16.0f;
  float dotX = centerX + dotDistance * std::cos(indicatorAngle);
  float dotY = centerY + dotDistance * std::sin(indicatorAngle);

  canvas.setColor(TextColor);
  canvas.circle(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f);
}

void GainKnob::mouseDown(const visage::MouseEvent& e)
{
  mIsDragging = true;
  mDragStartY = e.y;
  mDragStartValue = mValue;
}

void GainKnob::mouseDrag(const visage::MouseEvent& e)
{
  if (mIsDragging)
  {
    int dy = mDragStartY - e.y;
    double sensitivity = 0.005;
    double newValue = mDragStartValue + dy * sensitivity;
    newValue = std::max(0.0, std::min(1.0, newValue));

    if (newValue != mValue)
    {
      mValue = newValue;
      mOnValueChange.call(mValue);
      redraw();
    }
  }
}

void GainKnob::mouseUp(const visage::MouseEvent& e)
{
  mIsDragging = false;
}

void GainKnob::setValue(double value)
{
  mValue = std::max(0.0, std::min(1.0, value));
  redraw();
}

// ============================================================================
// NeuroBoost plugin
// ============================================================================

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
    if (mEditor)
      mEditor->redraw();
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

  mEditor->onDraw() = [this](visage::Canvas& canvas) {
    canvas.setColor(BackgroundColor);
    canvas.fill(0, 0, mEditor->width(), mEditor->height());

    canvas.setColor(TextColor);
    visage::Font titleFont(24);
    canvas.text("NeuroBoost", titleFont, visage::Font::kCenter,
                0.0f, 20.0f, static_cast<float>(mEditor->width()), 40.0f);

    // Show current generation mode
    visage::Font labelFont(16);
    int modeIdx = static_cast<int>(GetParam(kGenMode)->Value());
    const char* modeNames[] = {
      "Euclidean", "Fibonacci", "L-System",
      "Cellular Automata", "Markov", "Fractal", "Probability"
    };
    const char* modeName = (modeIdx >= 0 && modeIdx <= 6) ? modeNames[modeIdx] : "Unknown";
    canvas.text(modeName, labelFont, visage::Font::kCenter,
                0.0f, static_cast<float>(mEditor->height()) - 50.0f,
                static_cast<float>(mEditor->width()), 30.0f);

    // Step display placeholder
    std::ostringstream oss;
    oss << "Step: " << (mEngine.getCurrentStep() + 1)
        << " / " << static_cast<int>(GetParam(kStepCount)->Value());
    canvas.text(oss.str().c_str(), labelFont, visage::Font::kCenter,
                0.0f, static_cast<float>(mEditor->height()) - 25.0f,
                static_cast<float>(mEditor->width()), 25.0f);
  };

  using namespace visage::dimension;

  mGainKnob = std::make_unique<GainKnob>();
  mEditor->addChild(mGainKnob.get());

  float centerX = (static_cast<float>(mEditor->width())  - kKnobSize) / 2.0f;
  float centerY = (static_cast<float>(mEditor->height()) - kKnobSize) / 2.0f;
  mGainKnob->setBounds(centerX, centerY, kKnobSize, kKnobSize);

  // Knob controls Global Density (0.0-1.0) temporarily until Sprint 3 UI
  mGainKnob->setValue(GetParam(kGlobalDensity)->Value());

  mGainKnob->onValueChange() = [this](double value) {
    GetParam(kGlobalDensity)->Set(value);
    SendParameterValueFromUI(kGlobalDensity,
                             GetParam(kGlobalDensity)->GetNormalized());
    mEditor->redraw();
  };

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

    if (mGainKnob)
    {
      float centerX = (static_cast<float>(width)  - kKnobSize) / 2.0f;
      float centerY = (static_cast<float>(height) - kKnobSize) / 2.0f;
      mGainKnob->setBounds(centerX, centerY, kKnobSize, kKnobSize);
    }
  }
}

void NeuroBoost::CloseWindow()
{
  if (mEditor)
  {
    mEditor->removeFromWindow();
    mEditor->removeAllChildren();
  }
  mGainKnob.reset();
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

