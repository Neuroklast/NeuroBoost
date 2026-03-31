#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "visage_app/application_editor.h"
#include "visage_ui/frame.h"
#include "visage_windowing/windowing.h"

#include "dsp/SequencerEngine.h"
#include "dsp/LockFreeQueue.h"

const int kNumPresets = 1;

enum EParams
{
  kStepCount = 0,
  kGenMode,
  kRootNote,
  kScaleMode,
  kGlobalDensity,
  kSwing,
  kEuclideanHits,
  kEuclideanRotation,
  kFractalSeed,
  kFractalDimension,
  kFractalDepth,
  kMutationRate,
  kVelocityCurve,
  kOctaveLow,
  kOctaveHigh,
  kNumParams
};

// Constants for UI
static constexpr float kKnobSize = 120.0f;

// Custom knob control for Visage (kept for Sprint 3 UI redesign)
class GainKnob : public visage::Frame
{
public:
  GainKnob();

  void draw(visage::Canvas& canvas) override;
  void mouseDown(const visage::MouseEvent& e) override;
  void mouseDrag(const visage::MouseEvent& e) override;
  void mouseUp(const visage::MouseEvent& e) override;

  void setValue(double value);
  double getValue() const { return mValue; }

  auto& onValueChange() { return mOnValueChange; }

private:
  double mValue = 0.5;
  double mDragStartValue = 0.0;
  int mDragStartY = 0;
  bool mIsDragging = false;
  visage::CallbackList<void(double)> mOnValueChange;
};

class NeuroBoost final : public iplug::Plugin
{
public:
  NeuroBoost(const InstanceInfo& info);

  void OnIdle() override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;

  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  void OnParentWindowResize(int width, int height) override;

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;

private:
  SequencerEngine                mEngine;
  LockFreeQueue<int, 64>         mPlayheadQueue;

  std::unique_ptr<visage::ApplicationEditor> mEditor;
  std::unique_ptr<visage::Window>            mWindow;
  std::unique_ptr<GainKnob>                  mGainKnob;
};
