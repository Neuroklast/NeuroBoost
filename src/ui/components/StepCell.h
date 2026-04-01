#pragma once

#include "visage_ui/frame.h"
#include "visage_graphics/canvas.h"
#include <functional>
#include <cstdint>

/// Individual step cell in the step sequencer grid.
/// Displays the active state, velocity, and playhead position.
/// Supports click-to-toggle, vertical drag for velocity, and
/// a brief flash animation when the step is triggered.
class StepCell : public visage::Frame
{
public:
  StepCell();

  void draw(visage::Canvas& canvas) override;
  void mouseDown(const visage::MouseEvent& e) override;
  void mouseDrag(const visage::MouseEvent& e) override;
  void mouseUp(const visage::MouseEvent& e) override;

  // -----------------------------------------------------------------------
  // State setters (called by StepGrid)
  // -----------------------------------------------------------------------

  void setActive(bool active);
  bool isActive() const { return mActive; }

  /// Normalised velocity 0.0 – 1.0.
  void setVelocity(float velocity);
  float getVelocity() const { return mVelocity; }

  /// Normalised probability 0.0 – 1.0.
  void setProbability(float probability);
  float getProbability() const { return mProbability; }

  /// Highlight this cell as the current playhead position.
  void setPlayhead(bool isPlayhead);
  bool isPlayhead() const { return mIsPlayhead; }

  /// Highlight this cell as the currently selected step (keyboard navigation).
  void setSelected(bool selected);
  bool isSelected() const { return mIsSelected; }

  /// Trigger the brief flash animation (called when the step fires).
  void triggerFlash();

  /// Advance flash timer; should be called each frame from parent.
  /// Returns true when the cell still needs repainting.
  bool tickFlash(float deltaMs);

  // -----------------------------------------------------------------------
  // Callbacks
  // -----------------------------------------------------------------------

  /// Called when the user toggles active state: void(bool active)
  auto& onToggle() { return mOnToggle; }

  /// Called when the user drags to change velocity: void(float velocity)
  auto& onVelocityChange() { return mOnVelocityChange; }

  /// Called when shift+drag changes probability: void(float probability)
  auto& onProbabilityChange() { return mOnProbabilityChange; }

  /// Called when right-click toggles accent: void(bool accent)
  auto& onAccentToggle() { return mOnAccentToggle; }

  bool isAccent() const { return mAccent; }
  void setAccent(bool accent);

private:
  bool  mActive       = false;
  float mVelocity     = 1.0f;
  float mProbability  = 1.0f;
  bool  mIsPlayhead   = false;
  bool  mAccent       = false;
  bool  mIsSelected   = false;

  // Flash animation state
  static constexpr float kFlashDurationMs = 80.0f;
  float mFlashTimer = 0.0f;  // counts down from kFlashDurationMs to 0

  // Drag state
  int   mDragStartY          = 0;
  float mDragStartVelocity   = 0.0f;
  float mDragStartProbability = 0.0f;
  bool  mIsDragging          = false;
  bool  mIsShiftDrag         = false;

  visage::CallbackList<void(bool)>  mOnToggle;
  visage::CallbackList<void(float)> mOnVelocityChange;
  visage::CallbackList<void(float)> mOnProbabilityChange;
  visage::CallbackList<void(bool)>  mOnAccentToggle;
};
