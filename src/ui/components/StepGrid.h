#pragma once

#include "visage_ui/frame.h"
#include "visage_graphics/canvas.h"
#include "StepCell.h"
#include "../../common/Constants.h"
#include <array>
#include <cstdint>

/// Step sequencer grid component (up to MAX_STEPS cells, displayed 8 columns wide).
/// Manages the array of StepCell children and the playhead highlight.
class StepGrid : public visage::Frame
{
public:
  static constexpr int kColumns = 8;

  StepGrid();

  void draw(visage::Canvas& canvas) override;
  void resized() override;

  // -----------------------------------------------------------------------
  // State
  // -----------------------------------------------------------------------

  /// Set the number of active steps (1 – MAX_STEPS).
  void setStepCount(int count);
  int  getStepCount() const { return mStepCount; }

  /// Update the current playhead position (0-based).
  void setPlayheadStep(int step);

  /// Set active state of a single step.
  void setStepActive(int step, bool active);

  /// Set velocity of a single step (0.0 – 1.0).
  void setStepVelocity(int step, float velocity);

  /// Set probability of a single step (0.0 – 1.0).
  void setStepProbability(int step, float probability);

  /// Trigger the flash animation for a step (called when it fires).
  void triggerStepFlash(int step);

  /// Set the selected step (keyboard navigation). -1 = no selection.
  void setSelectedStep(int step);
  int  getSelectedStep() const { return mSelectedStep; }

  // -----------------------------------------------------------------------
  // Callbacks
  // -----------------------------------------------------------------------

  /// Fired when a step is toggled: void(int stepIndex, bool active)
  auto& onStepToggled() { return mOnStepToggled; }

  /// Fired when velocity is changed via drag: void(int stepIndex, float velocity)
  auto& onVelocityChanged() { return mOnVelocityChanged; }

  /// Fired when probability is changed via shift+drag: void(int stepIndex, float probability)
  auto& onProbabilityChanged() { return mOnProbabilityChanged; }

  /// Fired when accent is toggled via right-click: void(int stepIndex, bool accent)
  auto& onAccentToggled() { return mOnAccentToggled; }

private:
  int mStepCount    = 16;
  int mPlayheadStep = -1;
  int mSelectedStep = -1;  // -1 = no selection

  std::array<StepCell, MAX_STEPS> mCells;

  visage::CallbackList<void(int, bool)>  mOnStepToggled;
  visage::CallbackList<void(int, float)> mOnVelocityChanged;
  visage::CallbackList<void(int, float)> mOnProbabilityChanged;
  visage::CallbackList<void(int, bool)>  mOnAccentToggled;

  void layoutCells();
};
