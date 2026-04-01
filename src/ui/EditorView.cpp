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

  // -- Step grid --
  mStepGrid = std::make_unique<StepGrid>();
  mStepGrid->setStepCount(static_cast<int>(mGetParam(kStepCount)));
  mStepGrid->onStepToggled() = [this](int step, bool active) {
    if (mOnStepActiveChanged)
      mOnStepActiveChanged(step, active);
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

  float hdrH       = kHeaderH     * scaleY;
  float gridW      = kGridW       * scaleX;
  float gridH      = kGridH       * scaleY;
  float paramH     = kParamPanelH * scaleY;
  float transportH = kTransportH  * scaleY;

  // Header selectors
  float selW = 150.0f * scaleX;
  float selH = 28.0f  * scaleY;
  float selY = (hdrH - selH) / 2.0f;

  float genModeX = W / 2.0f - selW * 1.5f - 8.0f;
  mGenModeSelector->setBounds(genModeX, selY, selW, selH);
  mScaleModeSelector->setBounds(genModeX + selW + 8.0f, selY, selW, selH);
  mRootNoteSelector->setBounds(genModeX + (selW + 8.0f) * 2.0f, selY, 80.0f * scaleX, selH);

  // Step grid
  mStepGrid->setBounds(0.0f, hdrH, gridW, gridH);

  // Parameter panel
  float panelY = hdrH + gridH;
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

  float hdrH   = kHeaderH * scaleY;
  float gridH  = kGridH   * scaleY;
  float transH = kTransportH * scaleY;

  // Overall background
  canvas.setColor(BackgroundColor);
  canvas.fill(0.0f, 0.0f, W, H);

  // Header background
  canvas.setColor(PanelColor);
  canvas.fill(0.0f, 0.0f, W, hdrH);

  // Header title
  visage::Font titleFont(22);
  canvas.setColor(AccentColor);
  canvas.text("NeuroBoost", titleFont, visage::Font::kLeft,
              16.0f, 0.0f, 160.0f, hdrH);

  drawHeader(canvas);

  // Divider between grid and parameter panel
  float panelY = hdrH + gridH;
  canvas.setColor(SelectorBorderColor);
  canvas.fill(0.0f, panelY, W, 1.0f);

  // Parameter panel groups
  float scaleX = W / 900.0f;
  float groupW = W / 3.0f;
  float groupH = kParamPanelH * scaleY - 1.0f;
  drawPanelGroup(canvas, 0.0f,        panelY + 1.0f, groupW,     groupH, "RHYTHM");
  drawPanelGroup(canvas, groupW,      panelY + 1.0f, groupW,     groupH, "MELODY");
  drawPanelGroup(canvas, groupW*2.0f, panelY + 1.0f, groupW,     groupH, "PERFORMANCE");

  // Thin border around step grid area
  canvas.setColor(SelectorBorderColor);
  canvas.fill(0.0f, hdrH, kGridW * scaleX, 1.0f);
  canvas.fill(kGridW * scaleX, hdrH, 1.0f, gridH);
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

  visage::Font groupFont(11);
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
