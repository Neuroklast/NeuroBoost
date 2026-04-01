#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugMidi.h"
#include "visage_app/application_editor.h"
#include "visage_ui/frame.h"
#include "visage_windowing/windowing.h"

#include "dsp/SequencerEngine.h"
#include "dsp/LockFreeQueue.h"
#include "ui/EditorView.h"
#include "common/Presets.h"
#include "common/Types.h"

// Import iPlug2 types used in the plugin class declaration
using iplug::Plugin;
using iplug::InstanceInfo;
using iplug::IMidiMsg;
using iplug::IMidiQueue;
using iplug::sample;

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
  kMidiInputMode,
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
  void ProcessMidiMsg(const IMidiMsg& msg) override;

private:
  void sendPanicNoteOffs();
  void handleMidiInput(const IMidiMsg& msg);

  SequencerEngine                mEngine;
  LockFreeQueue<int, 64>         mPlayheadQueue;

  // MIDI input state
  IMidiQueue                     mMidiQueue;
  int                            mLastInputNote   = 60;
  int                            mHeldNoteCount   = 0;
  bool                           mPresetDirty     = false;

  std::unique_ptr<visage::ApplicationEditor> mEditor;
  std::unique_ptr<visage::Window>            mWindow;
  std::unique_ptr<EditorView>                mEditorView;
};
