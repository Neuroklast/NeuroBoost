#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "visage_app/application_editor.h"
#include "visage_ui/frame.h"
#include "visage_windowing/windowing.h"

#include "dsp/SequencerEngine.h"
#include "dsp/LockFreeQueue.h"
#include "ui/EditorView.h"
#include "common/Presets.h"

const int kNumPresets = kNumFactoryPresets;

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

class NeuroBoost final : public iplug::Plugin
{
public:
  NeuroBoost(const InstanceInfo& info);

  void OnIdle() override;
  void OnReset() override;
  void OnActivate(bool active) override;
  void OnParamChange(int paramIdx) override;

  bool SerializeState(iplug::IByteChunk& chunk) const override;
  int  UnserializeState(const iplug::IByteChunk& chunk, int startPos) override;

  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  void OnParentWindowResize(int width, int height) override;

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;

private:
  void sendPanicNoteOffs();

  SequencerEngine                mEngine;
  LockFreeQueue<int, 64>         mPlayheadQueue;

  std::unique_ptr<visage::ApplicationEditor> mEditor;
  std::unique_ptr<visage::Window>            mWindow;
  std::unique_ptr<EditorView>                mEditorView;
};
