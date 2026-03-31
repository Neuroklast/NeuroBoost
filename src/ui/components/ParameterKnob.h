#pragma once

#include "visage_ui/frame.h"
#include "visage_graphics/canvas.h"
#include "visage_utils/string_utils.h"
#include <string>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/// Reusable parameter knob component.
/// Displays a circular arc knob with a label below and the current value.
/// Evolved from the original GainKnob; now generic for any parameter.
class ParameterKnob : public visage::Frame
{
public:
  /// @param label     Display label shown below the knob arc.
  /// @param minVal    Minimum parameter value.
  /// @param maxVal    Maximum parameter value.
  /// @param defaultVal Default / reset value.
  ParameterKnob(std::string label, double minVal, double maxVal, double defaultVal);

  void draw(visage::Canvas& canvas) override;
  void mouseDown(const visage::MouseEvent& e) override;
  void mouseDrag(const visage::MouseEvent& e) override;
  void mouseUp(const visage::MouseEvent& e) override;
  void mouseDoubleClick(const visage::MouseEvent& e) override;
  void mouseWheelMove(const visage::MouseEvent& e, float deltaY) override;

  /// Set the knob value (clamped to [minVal, maxVal]).
  void setValue(double value);

  /// Returns the current knob value in [minVal, maxVal].
  double getValue() const { return mValue; }

  /// Returns the normalised value in [0, 1].
  double getNormalized() const;

  /// Set from a normalised [0, 1] value.
  void setNormalized(double normalized);

  /// Callback fired on every value change: void(double value).
  auto& onValueChange() { return mOnValueChange; }

private:
  std::string mLabel;
  double      mMinVal;
  double      mMaxVal;
  double      mDefaultVal;
  double      mValue;

  double mDragStartValue = 0.0;
  int    mDragStartY     = 0;
  bool   mIsDragging     = false;
  bool   mShiftHeld      = false;

  visage::CallbackList<void(double)> mOnValueChange;

  /// Format the current value for display.
  std::string formatValue() const;
};
