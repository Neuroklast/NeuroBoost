#include "ParameterKnob.h"
#include "../theme/Theme.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

ParameterKnob::ParameterKnob(std::string label, double minVal, double maxVal, double defaultVal)
  : mLabel(std::move(label))
  , mMinVal(minVal)
  , mMaxVal(maxVal)
  , mDefaultVal(defaultVal)
  , mValue(defaultVal)
{
}

void ParameterKnob::draw(visage::Canvas& canvas)
{
  float w = width();
  float h = height();
  float knobArea = h - 20.0f;  // reserve 20 px at bottom for label + value
  float size     = std::min(w, knobArea);
  float centerX  = w / 2.0f;
  float centerY  = knobArea / 2.0f;
  float radius   = (size - 8.0f) / 2.0f;

  // Knob background circle
  canvas.setColor(KnobBackgroundColor);
  canvas.circle(centerX - radius, centerY - radius, radius * 2.0f);

  float arcDiameter = (radius - 6.0f) * 2.0f;
  float thickness   = 5.0f;
  float startAngle  = static_cast<float>(5.0 * M_PI / 4.0);
  float fullSweep   = static_cast<float>(3.0 * M_PI / 2.0);

  // Track arc (full range, muted color)
  canvas.setColor(KnobTrackColor);
  canvas.flatArc(centerX - arcDiameter / 2.0f - thickness / 2.0f,
                 centerY - arcDiameter / 2.0f - thickness / 2.0f,
                 arcDiameter + thickness, thickness,
                 startAngle + fullSweep / 2.0f, fullSweep);

  // Value arc (colored)
  float normalised   = static_cast<float>(getNormalized());
  float valueSweep   = fullSweep * normalised;
  if (valueSweep > 0.01f)
  {
    canvas.setColor(KnobArcColor);
    canvas.flatArc(centerX - arcDiameter / 2.0f - thickness / 2.0f,
                   centerY - arcDiameter / 2.0f - thickness / 2.0f,
                   arcDiameter + thickness, thickness,
                   startAngle + valueSweep / 2.0f, valueSweep);
  }

  // Indicator dot
  float indicatorAngle = startAngle + valueSweep;
  float dotRadius      = 3.0f;
  float dotDistance    = radius - 12.0f;
  float dotX = centerX + dotDistance * std::cos(indicatorAngle);
  float dotY = centerY + dotDistance * std::sin(indicatorAngle);

  canvas.setColor(TextColor);
  canvas.circle(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f);

  // Label text
  visage::Font labelFont(11);
  canvas.setColor(TextDimColor);
  canvas.text(mLabel.c_str(), labelFont, visage::Font::kCenter,
              0.0f, knobArea, w, 12.0f);

  // Value text
  visage::Font valueFont(10);
  canvas.setColor(TextColor);
  std::string valStr = formatValue();
  canvas.text(valStr.c_str(), valueFont, visage::Font::kCenter,
              0.0f, knobArea + 13.0f, w, 10.0f);
}

void ParameterKnob::mouseDown(const visage::MouseEvent& e)
{
  mIsDragging    = true;
  mDragStartY    = e.y;
  mDragStartValue = mValue;
  mShiftHeld     = e.isShiftDown();
}

void ParameterKnob::mouseDrag(const visage::MouseEvent& e)
{
  if (!mIsDragging)
    return;

  int dy = mDragStartY - e.y;
  bool shift = e.isShiftDown();
  double sensitivity = shift ? 0.0005 : 0.005;
  double range       = mMaxVal - mMinVal;

  double newValue = mDragStartValue + dy * sensitivity * range;
  newValue = std::max(mMinVal, std::min(mMaxVal, newValue));

  if (newValue != mValue)
  {
    mValue = newValue;
    mOnValueChange.call(mValue);
    redraw();
  }
}

void ParameterKnob::mouseUp(const visage::MouseEvent& /*e*/)
{
  mIsDragging = false;
}

void ParameterKnob::mouseDoubleClick(const visage::MouseEvent& /*e*/)
{
  if (mValue != mDefaultVal)
  {
    mValue = mDefaultVal;
    mOnValueChange.call(mValue);
    redraw();
  }
}

void ParameterKnob::mouseWheelMove(const visage::MouseEvent& e, float deltaY)
{
  double range = mMaxVal - mMinVal;
  bool shift   = e.isShiftDown();
  double step  = shift ? range * 0.001 : range * 0.01;
  double newValue = std::max(mMinVal, std::min(mMaxVal, mValue + deltaY * step));

  if (newValue != mValue)
  {
    mValue = newValue;
    mOnValueChange.call(mValue);
    redraw();
  }
}

void ParameterKnob::setValue(double value)
{
  mValue = std::max(mMinVal, std::min(mMaxVal, value));
  redraw();
}

double ParameterKnob::getNormalized() const
{
  double range = mMaxVal - mMinVal;
  if (range <= 0.0)
    return 0.0;
  return (mValue - mMinVal) / range;
}

void ParameterKnob::setNormalized(double normalized)
{
  setValue(mMinVal + std::max(0.0, std::min(1.0, normalized)) * (mMaxVal - mMinVal));
}

std::string ParameterKnob::formatValue() const
{
  std::ostringstream oss;
  double range = mMaxVal - mMinVal;
  if (range >= 100.0)
    oss << std::fixed << std::setprecision(0) << mValue;
  else if (range >= 10.0)
    oss << std::fixed << std::setprecision(1) << mValue;
  else
    oss << std::fixed << std::setprecision(2) << mValue;
  return oss.str();
}
