#include "EditorView.h"
#include "theme/Theme.h"
#include "../NeuroBoost.h"  // EParams enum

EditorView::EditorView(std::function<double(int)> getParam,
                       std::function<void(int, double)> setParam)
  : mGetParam(std::move(getParam))
  , mSetParam(std::move(setParam))
{
  buildWidgets();
}

// ============================================================================
// Widget construction
// ============================================================================

void EditorView::buildWidgets()
{
  // -- Preset browser --
  mPresetBrowser = std::make_unique<PresetBrowser>();
  mPresetBrowser->mOnPresetSelected = [this](int idx) {
    if (mOnPresetSelected) mOnPresetSelected(idx);
  };
  addChild(mPresetBrowser.get());

  // -- Header: Generation mode selector --
  mGenModeSelector = std::make_unique<ModeSelector>();
  mGenModeSelector->setOptions({
    "Euclidean", "Fibonacci", "L-System",
    "Cellular Automata", "Markov", "Fractal", "Probability"
  });
  mGenModeSelector->setSelectedIndex(static_cast<int>(mGetParam(kGenMode)));
  mGenModeSelector->onSelectionChanged() = [this](int idx) {
    mSetParam(kGenMode, static_cast<double>(idx));
  };
  addChild(mGenModeSelector.get());

  // -- Header: Scale mode selector --
  mScaleModeSelector = std::make_unique<ModeSelector>();
  mScaleModeSelector->setOptions({
    "Chromatic", "Major", "Minor", "Dorian", "Phrygian",
    "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor",
    "Pentatonic Maj", "Pentatonic Min", "Blues",
    "Whole Tone", "Diminished", "Augmented"
  });
  mScaleModeSelector->setSelectedIndex(static_cast<int>(mGetParam(kScaleMode)));
  mScaleModeSelector->onSelectionChanged() = [this](int idx) {
    mSetParam(kScaleMode, static_cast<double>(idx));
  };
  addChild(mScaleModeSelector.get());

  // -- Header: Root note selector --
  mRootNoteSelector = std::make_unique<ModeSelector>();
  mRootNoteSelector->setOptions({
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
  });
  mRootNoteSelector->setSelectedIndex(
    static_cast<int>(mGetParam(kRootNote)) % 12);
  mRootNoteSelector->onSelectionChanged() = [this](int idx) {
    // Map semitone class back to MIDI note near C4 (60)
    int octave = static_cast<int>(mGetParam(kRootNote)) / 12;
    mSetParam(kRootNote, static_cast<double>(octave * 12 + idx));
  };
  addChild(mRootNoteSelector.get());

  // -- Header: MIDI Input Mode selector --
  mMidiModeSelector = std::make_unique<ModeSelector>();
  mMidiModeSelector->setOptions({
    "MIDI: Off", "MIDI: Thru", "MIDI: Transpose",
    "MIDI: Trigger", "MIDI: Gate", "MIDI: Thru+Gen"
  });
  mMidiModeSelector->setSelectedIndex(static_cast<int>(mGetParam(kMidiInputMode)));
  mMidiModeSelector->onSelectionChanged() = [this](int idx) {
    mSetParam(kMidiInputMode, static_cast<double>(idx));
  };
  addChild(mMidiModeSelector.get());

  // -- Step grid --
  mStepGrid = std::make_unique<StepGrid>();
  mStepGrid->setStepCount(static_cast<int>(mGetParam(kStepCount)));
  mStepGrid->onStepToggled() = [this](int step, bool active) {
    if (mOnStepActiveChanged)
      mOnStepActiveChanged(step, active);
  };
  mStepGrid->onVelocityChanged() = [this](int step, float velocity) {
    if (mOnStepVelocityChanged)
      mOnStepVelocityChanged(step, static_cast<double>(velocity));
  };
  mStepGrid->onProbabilityChanged() = [this](int step, float prob) {
    if (mOnStepProbabilityChanged)
      mOnStepProbabilityChanged(step, static_cast<double>(prob));
  };
  mStepGrid->onAccentToggled() = [this](int step, bool accent) {
    if (mOnStepAccentToggled)
      mOnStepAccentToggled(step, accent);
  };
  addChild(mStepGrid.get());

  // -- Rhythm knobs --
  mStepsKnob  = std::make_unique<ParameterKnob>("STEPS",  1.0, 64.0, 16.0);
  mHitsKnob   = std::make_unique<ParameterKnob>("HITS",   0.0, 64.0,  4.0);
  mRotateKnob = std::make_unique<ParameterKnob>("ROTATE", 0.0, 63.0,  0.0);

  mStepsKnob->setValue(mGetParam(kStepCount));
  mStepsKnob->onValueChange() = [this](double v) {
    mSetParam(kStepCount, v);
    mStepGrid->setStepCount(static_cast<int>(v));
  };

  mHitsKnob->setValue(mGetParam(kEuclideanHits));
  mHitsKnob->onValueChange() = [this](double v) {
    mSetParam(kEuclideanHits, v);
  };

  mRotateKnob->setValue(mGetParam(kEuclideanRotation));
  mRotateKnob->onValueChange() = [this](double v) {
    mSetParam(kEuclideanRotation, v);
  };

  addChild(mStepsKnob.get());
  addChild(mHitsKnob.get());
  addChild(mRotateKnob.get());

  // -- Melody knobs --
  mOctaveLowKnob  = std::make_unique<ParameterKnob>("OCT LOW",  0.0, 8.0, 3.0);
  mOctaveHighKnob = std::make_unique<ParameterKnob>("OCT HIGH", 0.0, 8.0, 5.0);
  mDensityKnob    = std::make_unique<ParameterKnob>("DENSITY",  0.0, 1.0, 1.0);

  mOctaveLowKnob->setValue(mGetParam(kOctaveLow));
  mOctaveLowKnob->onValueChange() = [this](double v) {
    mSetParam(kOctaveLow, v);
  };

  mOctaveHighKnob->setValue(mGetParam(kOctaveHigh));
  mOctaveHighKnob->onValueChange() = [this](double v) {
    mSetParam(kOctaveHigh, v);
  };

  mDensityKnob->setValue(mGetParam(kGlobalDensity));
  mDensityKnob->onValueChange() = [this](double v) {
    mSetParam(kGlobalDensity, v);
  };

  addChild(mOctaveLowKnob.get());
  addChild(mOctaveHighKnob.get());
  addChild(mDensityKnob.get());

  // -- Performance knobs --
  mSwingKnob        = std::make_unique<ParameterKnob>("SWING",    0.0, 1.0, 0.0);
  mMutationKnob     = std::make_unique<ParameterKnob>("MUTATION", 0.0, 1.0, 0.0);
  mVelocityCurveKnob = std::make_unique<ParameterKnob>("VEL CURVE", 0.1, 4.0, 1.0);

  mSwingKnob->setValue(mGetParam(kSwing));
  mSwingKnob->onValueChange() = [this](double v) {
    mSetParam(kSwing, v);
  };

  mMutationKnob->setValue(mGetParam(kMutationRate));
  mMutationKnob->onValueChange() = [this](double v) {
    mSetParam(kMutationRate, v);
  };

  mVelocityCurveKnob->setValue(mGetParam(kVelocityCurve));
  mVelocityCurveKnob->onValueChange() = [this](double v) {
    mSetParam(kVelocityCurve, v);
  };

  addChild(mSwingKnob.get());
  addChild(mMutationKnob.get());
  addChild(mVelocityCurveKnob.get());

  // -- Transport bar --
  mTransportBar = std::make_unique<TransportBar>();
  // Transport play/stop/reset are driven by the DAW; callbacks here can
  // be wired in the plugin host layer if internal transport is needed.
  addChild(mTransportBar.get());
}

// ============================================================================
// Layout
// ============================================================================

void EditorView::resized()
{
  layoutWidgets();
}

void EditorView::layoutWidgets()
{
  float W = static_cast<float>(width());
  float H = static_cast<float>(height());

  // Scale layout proportionally from the 900×600 reference size
  float scaleX = W / 900.0f;
  float scaleY = H / 600.0f;

  float presetH    = kPresetBarH  * scaleY;
  float hdrH       = kHeaderH     * scaleY;
  float gridW      = kGridW       * scaleX;
  float gridH      = kGridH       * scaleY;
  float transportH = kTransportH  * scaleY;

  // Preset browser (top strip)
  mPresetBrowser->setBounds(0.0f, 0.0f, W, presetH);

  float hdrY = presetH;

  // Header selectors
  float selW = 130.0f * scaleX;
  float selH = 28.0f  * scaleY;
  float selY = hdrY + (hdrH - selH) / 2.0f;

  float genModeX = 8.0f * scaleX;
  mGenModeSelector->setBounds(genModeX, selY, selW, selH);
  mScaleModeSelector->setBounds(genModeX + selW + 6.0f, selY, selW, selH);
  mRootNoteSelector->setBounds(genModeX + (selW + 6.0f) * 2.0f, selY, 60.0f * scaleX, selH);
  mMidiModeSelector->setBounds(genModeX + (selW + 6.0f) * 2.0f + 60.0f * scaleX + 6.0f,
                                selY, selW, selH);

  // Step grid
  mStepGrid->setBounds(0.0f, hdrY + hdrH, gridW, gridH);

  // Parameter panel
  float panelY = hdrY + hdrH + gridH;
  float panelW = W / 3.0f;
  float knobSize = 68.0f * std::min(scaleX, scaleY);
  float knobY    = panelY + 20.0f * scaleY;
  float knobPad  = 12.0f * scaleX;

  auto placeKnob = [&](ParameterKnob* k, float x, float y) {
    k->setBounds(x, y, knobSize, knobSize + 20.0f);
  };

  // Rhythm group: STEPS, HITS, ROTATE
  float rxBase = knobPad;
  placeKnob(mStepsKnob.get(),  rxBase,                           knobY);
  placeKnob(mHitsKnob.get(),   rxBase + knobSize + knobPad,      knobY);
  placeKnob(mRotateKnob.get(), rxBase + (knobSize + knobPad)*2,  knobY);

  // Melody group: OCT LOW, OCT HIGH, DENSITY
  float mxBase = panelW + knobPad;
  placeKnob(mOctaveLowKnob.get(),  mxBase,                           knobY);
  placeKnob(mOctaveHighKnob.get(), mxBase + knobSize + knobPad,      knobY);
  placeKnob(mDensityKnob.get(),    mxBase + (knobSize + knobPad)*2,  knobY);

  // Performance group: SWING, MUTATION, VEL CURVE
  float pxBase = panelW * 2.0f + knobPad;
  placeKnob(mSwingKnob.get(),         pxBase,                           knobY);
  placeKnob(mMutationKnob.get(),      pxBase + knobSize + knobPad,      knobY);
  placeKnob(mVelocityCurveKnob.get(), pxBase + (knobSize + knobPad)*2,  knobY);

  // Transport bar
  mTransportBar->setBounds(0.0f, H - transportH, W, transportH);
}

// ============================================================================
// Drawing
// ============================================================================

void EditorView::draw(visage::Canvas& canvas)
{
  float W = static_cast<float>(width());
  float H = static_cast<float>(height());
  float scaleY = H / 600.0f;
  float scaleX = W / 900.0f;

  float presetH = kPresetBarH  * scaleY;
  float hdrH    = kHeaderH     * scaleY;
  float gridH   = kGridH       * scaleY;

  // Overall background
  canvas.setColor(BackgroundColor);
  canvas.fill(0.0f, 0.0f, W, H);

  // Header background
  canvas.setColor(PanelColor);
  canvas.fill(0.0f, presetH, W, hdrH);

  // Header title
  visage::Font titleFont = makeFont(20.0f);
  canvas.setColor(AccentColor);
  canvas.text("NeuroBoost", titleFont, visage::Font::kLeft,
              16.0f, presetH, 160.0f, hdrH);

  drawHeader(canvas);

  // Divider between grid and parameter panel
  float panelY = presetH + hdrH + gridH;
  canvas.setColor(SelectorBorderColor);
  canvas.fill(0.0f, panelY, W, 1.0f);

  // Parameter panel groups
  float groupW = W / 3.0f;
  float groupH = kParamPanelH * scaleY - 1.0f;
  drawPanelGroup(canvas, 0.0f,        panelY + 1.0f, groupW,     groupH, "RHYTHM");
  drawPanelGroup(canvas, groupW,      panelY + 1.0f, groupW,     groupH, "MELODY");
  drawPanelGroup(canvas, groupW*2.0f, panelY + 1.0f, groupW,     groupH, "PERFORMANCE");

  // Thin border around step grid area
  canvas.setColor(SelectorBorderColor);
  canvas.fill(0.0f, presetH + hdrH, kGridW * scaleX, 1.0f);
  canvas.fill(kGridW * scaleX, presetH + hdrH, 1.0f, gridH);
}

void EditorView::drawHeader(visage::Canvas& /*canvas*/)
{
  // Selectors draw themselves; nothing extra needed here.
}

void EditorView::drawPanelGroup(visage::Canvas& canvas,
                                float x, float y, float w, float h,
                                const char* title)
{
  canvas.setColor(PanelColor);
  canvas.fill(x + 4.0f, y + 4.0f, w - 8.0f, h - 8.0f);

  // Border (top line)
  canvas.setColor(SelectorBorderColor);
  canvas.fill(x + 4.0f, y + 4.0f, w - 8.0f, 1.0f);

  visage::Font groupFont = makeFont(11.0f);
  canvas.setColor(AccentColor);
  canvas.text(title, groupFont, visage::Font::kCenter, x + 4.0f, y + 8.0f, w - 8.0f, 14.0f);
}

// ============================================================================
// Runtime updates
// ============================================================================

void EditorView::setPlayheadStep(int step)
{
  if (mStepGrid)
    mStepGrid->setPlayheadStep(step);
}

void EditorView::triggerStepFlash(int step)
{
  if (mStepGrid)
    mStepGrid->triggerStepFlash(step);
}

void EditorView::setPlaying(bool playing)
{
  if (mTransportBar)
    mTransportBar->setPlaying(playing);
}

void EditorView::updateKnobFromHost(int paramIdx, double value)
{
  // Map parameter index to the appropriate knob and update display without feedback.
  ParameterKnob* knob = nullptr;
  switch (paramIdx)
  {
    case kStepCount:        knob = mStepsKnob.get();        break;
    case kEuclideanHits:    knob = mHitsKnob.get();         break;
    case kEuclideanRotation:knob = mRotateKnob.get();       break;
    case kOctaveLow:        knob = mOctaveLowKnob.get();    break;
    case kOctaveHigh:       knob = mOctaveHighKnob.get();   break;
    case kGlobalDensity:    knob = mDensityKnob.get();      break;
    case kSwing:            knob = mSwingKnob.get();        break;
    case kMutationRate:     knob = mMutationKnob.get();     break;
    case kVelocityCurve:    knob = mVelocityCurveKnob.get();break;
    case kGenMode:
      if (mGenModeSelector)
        mGenModeSelector->setSelectedIndex(static_cast<int>(value));
      return;
    case kScaleMode:
      if (mScaleModeSelector)
        mScaleModeSelector->setSelectedIndex(static_cast<int>(value));
      return;
    case kRootNote:
      if (mRootNoteSelector)
        mRootNoteSelector->setSelectedIndex(static_cast<int>(value) % 12);
      return;
    case kMidiInputMode:
      if (mMidiModeSelector)
        mMidiModeSelector->setSelectedIndex(static_cast<int>(value));
      return;
    default:
      return;
  }
  if (knob)
    knob->setValueFromHost(value);
}

void EditorView::updatePresetBrowser(int currentPresetIdx, bool dirty)
{
  if (mPresetBrowser)
  {
    mPresetBrowser->setCurrentPreset(currentPresetIdx);
    mPresetBrowser->setDirty(dirty);
  }
}

void EditorView::setPresetNames(std::vector<std::string> names)
{
  if (mPresetBrowser)
    mPresetBrowser->setPresetNames(std::move(names));
}

// ============================================================================
// Keyboard shortcuts (Goal 5)
// ============================================================================

bool EditorView::keyDown(const visage::KeyEvent& e)
{
  const int stepCount = mStepGrid ? mStepGrid->getStepCount() : 16;
  int sel = mStepGrid ? mStepGrid->getSelectedStep() : -1;

  // Space → toggle transport (play/pause)
  if (e.key_code == visage::KeyEvent::kSpace)
  {
    if (mTransportBar)
      mTransportBar->onPlayPause().callback();
    return true;
  }

  // 1–7 → switch generation mode
  if (e.key_code >= '1' && e.key_code <= '7')
  {
    int mode = e.key_code - '1';
    mSetParam(kGenMode, static_cast<double>(mode));
    if (mGenModeSelector)
      mGenModeSelector->setSelectedIndex(mode);
    return true;
  }

  // Arrow keys — move selection
  if (e.key_code == visage::KeyEvent::kLeftArrow)
  {
    if (sel <= 0) sel = stepCount - 1;
    else          sel = sel - 1;
    if (mStepGrid) mStepGrid->setSelectedStep(sel);
    return true;
  }
  if (e.key_code == visage::KeyEvent::kRightArrow)
  {
    sel = (sel < 0) ? 0 : (sel + 1) % stepCount;
    if (mStepGrid) mStepGrid->setSelectedStep(sel);
    return true;
  }

  // Up/Down — velocity ±10% on selected step
  if (e.key_code == visage::KeyEvent::kUpArrow && sel >= 0)
  {
    if (mOnStepVelocityChanged)
      mOnStepVelocityChanged(sel, 0.1);  // relative hint; plugin layer can interpret
    return true;
  }
  if (e.key_code == visage::KeyEvent::kDownArrow && sel >= 0)
  {
    if (mOnStepVelocityChanged)
      mOnStepVelocityChanged(sel, -0.1);
    return true;
  }

  // Delete / Backspace → clear selected step
  if ((e.key_code == visage::KeyEvent::kDelete ||
       e.key_code == visage::KeyEvent::kBackspace) && sel >= 0)
  {
    if (mOnStepActiveChanged)
      mOnStepActiveChanged(sel, false);
    if (mStepGrid)
      mStepGrid->setStepActive(sel, false);
    return true;
  }

  // A → toggle accent on selected step
  if ((e.key_code == 'A' || e.key_code == 'a') && sel >= 0)
  {
    if (mOnStepAccentToggled)
      mOnStepAccentToggled(sel, true);
    return true;
  }

  // R → randomize (re-trigger generation with new seed)
  if (e.key_code == 'R' || e.key_code == 'r')
  {
    double curSeed = mGetParam(kFractalSeed);
    mSetParam(kFractalSeed, std::fmod(curSeed + 1.0, 99999.0));
    return true;
  }

  // Ctrl+C / Cmd+C → copy pattern
  if ((e.key_code == 'C' || e.key_code == 'c') && e.isControlDown())
  {
    if (mOnCopyPattern) mOnCopyPattern();
    return true;
  }

  // E → export MIDI
  if (e.key_code == 'E' || e.key_code == 'e')
  {
    if (mOnExportMidi) mOnExportMidi();
    return true;
  }

  return false;
}
