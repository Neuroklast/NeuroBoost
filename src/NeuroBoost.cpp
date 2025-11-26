#include "NeuroBoost.h"
#include "IPlug_include_in_plug_src.h"
#include "visage_windowing/windowing.h"
#include "visage_utils/dimension.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

VISAGE_THEME_COLOR(BackgroundColor, 0xff2d2d2d);
VISAGE_THEME_COLOR(KnobBackgroundColor, 0xff404040);
VISAGE_THEME_COLOR(KnobArcColor, 0xff4a9eff);
VISAGE_THEME_COLOR(KnobTrackColor, 0xff555555);
VISAGE_THEME_COLOR(TextColor, 0xffffffff);

// GainKnob implementation
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
  
  // Draw background circle
  canvas.setColor(KnobBackgroundColor);
  canvas.circle(centerX - radius, centerY - radius, radius * 2.0f);
  
  // Draw the arc track (the full range from start to end)
  float arcDiameter = (radius - 8.0f) * 2.0f;
  float thickness = 6.0f;
  float startAngle = static_cast<float>(5.0 * M_PI / 4.0);  // 225 degrees in radians (bottom-left)
  float fullSweep = static_cast<float>(3.0 * M_PI / 2.0);   // 270 degrees sweep
  
  // Draw the track
  canvas.setColor(KnobTrackColor);
  canvas.flatArc(centerX - arcDiameter / 2.0f - thickness / 2.0f, 
                 centerY - arcDiameter / 2.0f - thickness / 2.0f,
                 arcDiameter + thickness, thickness, 
                 startAngle + fullSweep / 2.0f, fullSweep);
  
  // Draw the value arc
  float valueSweep = fullSweep * static_cast<float>(mValue);
  if (valueSweep > 0.01f) {
    canvas.setColor(KnobArcColor);
    canvas.flatArc(centerX - arcDiameter / 2.0f - thickness / 2.0f, 
                   centerY - arcDiameter / 2.0f - thickness / 2.0f,
                   arcDiameter + thickness, thickness, 
                   startAngle + valueSweep / 2.0f, valueSweep);
  }
  
  // Draw center indicator dot
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

// NeuroBoost implementation
NeuroBoost::NeuroBoost(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 200.0, 0.1, "%");
}

void NeuroBoost::OnIdle()
{
}

void* NeuroBoost::OpenWindow(void* pParent)
{
  mEditor = std::make_unique<visage::ApplicationEditor>();
  visage::IBounds bounds = visage::computeWindowBounds(PLUG_WIDTH, PLUG_HEIGHT);
  mEditor->setBounds(0, 0, bounds.width(), bounds.height());
  
  mEditor->onDraw() = [this](visage::Canvas& canvas) {
    canvas.setColor(BackgroundColor);
    canvas.fill(0, 0, mEditor->width(), mEditor->height());
    
    // Draw title
    canvas.setColor(TextColor);
    visage::Font titleFont(24);
    canvas.text("NeuroBoost", titleFont, visage::Font::kCenter, 
                0.0f, 20.0f, static_cast<float>(mEditor->width()), 40.0f);
    
    // Draw label
    visage::Font labelFont(16);
    canvas.text("Gain", labelFont, visage::Font::kCenter, 
                0.0f, static_cast<float>(mEditor->height()) - 50.0f, 
                static_cast<float>(mEditor->width()), 30.0f);
    
    // Draw value
    double gainValue = GetParam(kGain)->Value();
    char valueStr[32];
    snprintf(valueStr, sizeof(valueStr), "%.1f%%", gainValue);
    canvas.text(valueStr, labelFont, visage::Font::kCenter, 
                0.0f, static_cast<float>(mEditor->height()) - 30.0f, 
                static_cast<float>(mEditor->width()), 30.0f);
  };

  using namespace visage::dimension;
  
  mGainKnob = std::make_unique<GainKnob>();
  mEditor->addChild(mGainKnob.get());
  
  float knobSize = 120.0f;
  float centerX = (static_cast<float>(mEditor->width()) - knobSize) / 2.0f;
  float centerY = (static_cast<float>(mEditor->height()) - knobSize) / 2.0f;
  mGainKnob->setBounds(centerX, centerY, knobSize, knobSize);
  
  mGainKnob->setValue(GetParam(kGain)->Value() / 200.0);
  
  mGainKnob->onValueChange() = [this](double value) {
    double gainValue = value * 200.0;
    GetParam(kGain)->Set(gainValue);
    SendParameterValueFromUI(kGain, gainValue);
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
      float knobSize = 120.0f;
      float centerX = (static_cast<float>(width) - knobSize) / 2.0f;
      float centerY = (static_cast<float>(height) - knobSize) / 2.0f;
      mGainKnob->setBounds(centerX, centerY, knobSize, knobSize);
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
  
  IEditorDelegate::CloseWindow();
}

void NeuroBoost::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
