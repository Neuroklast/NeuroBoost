#pragma once

#include "visage_app/application_editor.h"
#include "visage_ui/frame.h"
#include "visage_graphics/canvas.h"
#include "components/StepGrid.h"
#include "components/ParameterKnob.h"
#include "components/ModeSelector.h"
#include "components/TransportBar.h"
#include "../dsp/SequencerEngine.h"
#include <memory>
#include <functional>

/// Main editor frame for the NeuroBoost sequencer plugin.
/// Owns all UI sub-components and wires them to the plugin parameters.
///
/// Layout (900 × 600):
///   - Header bar (0, 0, 900, 48): title + mode/scale/root selectors
///   - Step grid (0, 48, 600, 300): step sequencer cells
///   - Parameter panel (0, 348, 900, 212): knobs for all parameters
///   - Transport bar (0, 560, 900, 40): play/stop/reset
class EditorView : public visage::Frame
{
public:
  /// @param getParam   Functor returning a double parameter value by index.
  /// @param setParam   Functor called when a parameter changes: (paramIdx, value)
  explicit EditorView(std::function<double(int)> getParam,
                      std::function<void(int, double)> setParam);

  void draw(visage::Canvas& canvas) override;
  void resized() override;

  // -----------------------------------------------------------------------
  // Runtime updates from the audio thread (called from OnIdle)
  // -----------------------------------------------------------------------

  /// Update the playhead position indicator.
  void setPlayheadStep(int step);

  /// Trigger the flash animation for a step (step fired).
  void triggerStepFlash(int step);

  /// Reflect the current transport running state.
  void setPlaying(bool playing);

  /// Update a knob's visual display from a host automation change.
  /// Does NOT fire the onValueChange callback back to the engine.
  void updateKnobFromHost(int paramIdx, double value);

  // -----------------------------------------------------------------------
  // Layout constants (matches 900×600 default size)
  // -----------------------------------------------------------------------
  static constexpr float kHeaderH     = 48.0f;
  static constexpr float kGridW       = 600.0f;
  static constexpr float kGridH       = 300.0f;
  static constexpr float kParamPanelH = 212.0f;
  static constexpr float kTransportH  = 40.0f;

private:
  std::function<double(int)>       mGetParam;
  std::function<void(int, double)> mSetParam;
  std::function<void(int, bool)>   mOnStepActiveChanged;
  std::function<void(int, double)> mOnStepVelocityChanged;
  std::function<void(int, double)> mOnStepProbabilityChanged;
  std::function<void(int, bool)>   mOnStepAccentToggled;

public:
  void setOnStepActiveChanged(std::function<void(int, bool)> cb) { mOnStepActiveChanged = std::move(cb); }
  void setOnStepVelocityChanged(std::function<void(int, double)> cb) { mOnStepVelocityChanged = std::move(cb); }
  void setOnStepProbabilityChanged(std::function<void(int, double)> cb) { mOnStepProbabilityChanged = std::move(cb); }
  void setOnStepAccentToggled(std::function<void(int, bool)> cb) { mOnStepAccentToggled = std::move(cb); }

private:

  // Header widgets
  std::unique_ptr<ModeSelector> mGenModeSelector;
  std::unique_ptr<ModeSelector> mScaleModeSelector;
  std::unique_ptr<ModeSelector> mRootNoteSelector;

  // Step grid
  std::unique_ptr<StepGrid> mStepGrid;

  // Parameter knobs (rhythm group)
  std::unique_ptr<ParameterKnob> mStepsKnob;
  std::unique_ptr<ParameterKnob> mHitsKnob;
  std::unique_ptr<ParameterKnob> mRotateKnob;

  // Parameter knobs (melody group)
  std::unique_ptr<ParameterKnob> mOctaveLowKnob;
  std::unique_ptr<ParameterKnob> mOctaveHighKnob;
  std::unique_ptr<ParameterKnob> mDensityKnob;

  // Parameter knobs (performance group)
  std::unique_ptr<ParameterKnob> mSwingKnob;
  std::unique_ptr<ParameterKnob> mMutationKnob;
  std::unique_ptr<ParameterKnob> mVelocityCurveKnob;

  // Transport bar
  std::unique_ptr<TransportBar> mTransportBar;

  void buildWidgets();
  void layoutWidgets();

  void drawHeader(visage::Canvas& canvas);
  void drawPanelGroup(visage::Canvas& canvas,
                      float x, float y, float w, float h,
                      const char* title);
};
