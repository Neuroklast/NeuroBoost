#pragma once

#include "visage_ui/frame.h"
#include "visage_graphics/canvas.h"

/// Transport bar with Play, Stop, and Reset buttons.
/// Displays transport state visually and fires callbacks on interaction.
class TransportBar : public visage::Frame
{
public:
  TransportBar();

  void draw(visage::Canvas& canvas) override;
  void mouseDown(const visage::MouseEvent& e) override;

  // -----------------------------------------------------------------------
  // State
  // -----------------------------------------------------------------------

  void setPlaying(bool playing);
  bool isPlaying() const { return mIsPlaying; }

  // -----------------------------------------------------------------------
  // Callbacks
  // -----------------------------------------------------------------------

  /// Fired when the Play/Pause button is clicked.
  auto& onPlayPause() { return mOnPlay;  }

  /// Fired when the Stop button is clicked.
  auto& onStop()  { return mOnStop;  }

  /// Fired when the Reset button is clicked.
  auto& onReset() { return mOnReset; }

private:
  bool mIsPlaying = false;

  // Button hit-test rects (set during draw)
  float mPlayX   = 0.0f, mPlayW   = 0.0f;
  float mStopX   = 0.0f, mStopW   = 0.0f;
  float mResetX  = 0.0f, mResetW  = 0.0f;

  visage::CallbackList<void()> mOnPlay;
  visage::CallbackList<void()> mOnStop;
  visage::CallbackList<void()> mOnReset;

  void drawButton(visage::Canvas& canvas, const char* label,
                  float x, float y, float w, float h, bool active);
};
