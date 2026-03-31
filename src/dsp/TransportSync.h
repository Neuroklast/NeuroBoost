#pragma once

#include "../common/Constants.h"

// Host transport synchronisation helper.
// Realtime-safe: plain member variables only, no heap allocations.
class TransportSync
{
public:
  TransportSync()
    : mBlockStartBeat(0.0)
    , mBlockEndBeat(0.0)
    , mSamplesPerBeat(0.0)
    , mTempo(DEFAULT_TEMPO)
    , mSampleRate(44100.0)
    , mNFrames(0)
    , mWasPlaying(false)
    , mIsPlaying(false)
    , mTransportJustStarted(false)
    , mTransportJustStopped(false)
    , mLooped(false)
    , mLastPpqPos(0.0)
  {
  }

  // Call once per audio block with values from the host.
  // ppqPos    – PPQ position at the start of this block
  // tempo     – BPM
  // sampleRate – samples per second
  // isPlaying  – host transport state
  // nFrames   – block size in samples
  void update(double ppqPos, double tempo, double sampleRate,
              bool isPlaying, int nFrames)
  {
    // Clamp tempo to prevent division by zero
    if (tempo < MIN_TEMPO) tempo = MIN_TEMPO;
    if (tempo > MAX_TEMPO) tempo = MAX_TEMPO;

    mTempo      = tempo;
    mSampleRate = sampleRate;
    mNFrames    = nFrames;

    mSamplesPerBeat = sampleRate * 60.0 / tempo;

    mBlockStartBeat = ppqPos;
    mBlockEndBeat   = ppqPos + (nFrames > 0 ? static_cast<double>(nFrames) / mSamplesPerBeat : 0.0);

    // Transport state transitions
    mTransportJustStarted = (!mWasPlaying && isPlaying);
    mTransportJustStopped = (mWasPlaying && !isPlaying);

    // Loop detection: PPQ position jumped backwards during playback
    mLooped = (isPlaying && mWasPlaying && ppqPos < mLastPpqPos - 0.01);

    mWasPlaying  = isPlaying;
    mIsPlaying   = isPlaying;
    mLastPpqPos  = ppqPos;
  }

  // Beat position at the start of the current block.
  double getBlockStartBeat() const { return mBlockStartBeat; }

  // Beat position at the end of the current block.
  double getBlockEndBeat() const { return mBlockEndBeat; }

  // Samples per beat at the current tempo / sample rate.
  double getSamplesPerBeat() const { return mSamplesPerBeat; }

  // Convert an absolute beat position to a sample offset within the current block.
  // Returns -1 if beatPos is outside the current block.
  int beatToSampleOffset(double beatPos) const
  {
    if (mSamplesPerBeat <= 0.0) return -1;

    double offset = (beatPos - mBlockStartBeat) * mSamplesPerBeat;
    int sample = static_cast<int>(offset);

    if (sample < 0)         return -1;
    if (sample >= mNFrames) return -1;
    return sample;
  }

  // True on the first block after the transport transitions from stopped to playing.
  bool hasTransportJustStarted() const { return mTransportJustStarted; }

  // True on the first block after the transport transitions from playing to stopped.
  bool hasTransportJustStopped() const { return mTransportJustStopped; }

  // True when the PPQ position jumped backwards (loop point crossed).
  bool hasLooped() const { return mLooped; }

  // True while the host transport is playing.
  bool isPlaying() const { return mIsPlaying; }

  // Current tempo in BPM.
  double getTempo() const { return mTempo; }

  // Current sample rate.
  double getSampleRate() const { return mSampleRate; }

private:
  double mBlockStartBeat;
  double mBlockEndBeat;
  double mSamplesPerBeat;
  double mTempo;
  double mSampleRate;
  int    mNFrames;

  bool   mWasPlaying;
  bool   mIsPlaying;
  bool   mTransportJustStarted;
  bool   mTransportJustStopped;
  bool   mLooped;
  double mLastPpqPos;
};
