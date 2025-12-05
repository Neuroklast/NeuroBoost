#pragma once

// DSP-only version of NeuroBoost for testing without GUI dependencies

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring>

// Sample type
typedef float sample;

// Plugin parameter enumeration
enum EParams
{
  kGain = 0,
  kNumParams
};

// Simple DSP processor class (standalone, no iPlug2 dependency for testing)
class NeuroBoostDSP
{
public:
  NeuroBoostDSP()
    : mGain(1.0)
    , mSampleRate(44100.0)
  {
  }
  
  void SetSampleRate(double sampleRate)
  {
    mSampleRate = sampleRate;
  }
  
  void SetGain(double gainPercent)
  {
    // gainPercent: 0-200, where 100 = unity gain
    mGain = gainPercent / 100.0;
  }
  
  double GetGain() const
  {
    return mGain * 100.0;
  }
  
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames, int nChans)
  {
    for (int s = 0; s < nFrames; s++) {
      for (int c = 0; c < nChans; c++) {
        outputs[c][s] = inputs[c][s] * static_cast<sample>(mGain);
      }
    }
  }
  
  // Process mono buffer
  void ProcessMono(sample* buffer, int nFrames)
  {
    for (int s = 0; s < nFrames; s++) {
      buffer[s] *= static_cast<sample>(mGain);
    }
  }
  
  // Process stereo interleaved buffer
  void ProcessStereoInterleaved(sample* buffer, int nFrames)
  {
    for (int s = 0; s < nFrames * 2; s++) {
      buffer[s] *= static_cast<sample>(mGain);
    }
  }
  
private:
  double mGain;
  double mSampleRate;
};
