#include "TransportBar.h"
#include "../theme/Theme.h"

TransportBar::TransportBar()
{
}

void TransportBar::draw(visage::Canvas& canvas)
{
  float w  = width();
  float h  = height();
  float bw = 64.0f;
  float bh = h - 8.0f;
  float by = 4.0f;
  float gap = 8.0f;

  // Layout buttons left-to-right: [PLAY] [STOP] [RESET]
  mPlayW  = bw;
  mPlayX  = gap;
  mStopW  = bw;
  mStopX  = mPlayX + bw + gap;
  mResetW = bw;
  mResetX = mStopX + bw + gap;

  // Background
  canvas.setColor(PanelColor);
  canvas.fill(0.0f, 0.0f, w, h);

  drawButton(canvas, mIsPlaying ? "PAUSE" : "PLAY",  mPlayX,  by, mPlayW,  bh, mIsPlaying);
  drawButton(canvas, "STOP",  mStopX,  by, mStopW,  bh, false);
  drawButton(canvas, "RESET", mResetX, by, mResetW, bh, false);
}

void TransportBar::drawButton(visage::Canvas& canvas, const char* label,
                              float x, float y, float bw, float bh, bool active)
{
  if (active)
    canvas.setColor(PlayColor);
  else
    canvas.setColor(SelectorBgColor);
  canvas.fill(x, y, bw, bh);

  // Border lines
  canvas.setColor(SelectorBorderColor);
  canvas.fill(x, y, bw, 1.0f);
  canvas.fill(x, y + bh - 1.0f, bw, 1.0f);
  canvas.fill(x, y, 1.0f, bh);
  canvas.fill(x + bw - 1.0f, y, 1.0f, bh);

  visage::Font font(12);
  if (active)
    canvas.setColor(BackgroundColor);
  else
    canvas.setColor(TextColor);
  canvas.text(label, font, visage::Font::kCenter, x, y, bw, bh);
}

void TransportBar::mouseDown(const visage::MouseEvent& e)
{
  float h  = height();
  float by = 4.0f;
  float bh = h - 8.0f;

  auto hitTest = [&](float bx, float bw) {
    return e.x >= bx && e.x <= bx + bw && e.y >= by && e.y <= by + bh;
  };

  if (hitTest(mPlayX, mPlayW))
  {
    mOnPlay.call();
  }
  else if (hitTest(mStopX, mStopW))
  {
    mIsPlaying = false;
    mOnStop.call();
    redraw();
  }
  else if (hitTest(mResetX, mResetW))
  {
    mIsPlaying = false;
    mOnReset.call();
    redraw();
  }
}

void TransportBar::setPlaying(bool playing)
{
  if (mIsPlaying != playing)
  {
    mIsPlaying = playing;
    redraw();
  }
}
