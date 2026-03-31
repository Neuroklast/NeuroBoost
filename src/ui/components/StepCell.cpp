#include "StepCell.h"
#include "../theme/Theme.h"
#include <algorithm>

StepCell::StepCell()
{
}

void StepCell::draw(visage::Canvas& canvas)
{
  float w = width();
  float h = height();
  constexpr float kPad = 2.0f;

  // Flash overlay: interpolate toward white
  float flashT = (mFlashTimer > 0.0f) ? mFlashTimer / kFlashDurationMs : 0.0f;

  // Determine cell color name based on state
  if (mIsPlayhead)
    canvas.setColor(CellPlayheadColor);
  else if (mActive)
    canvas.setColor(CellActiveColor);
  else
    canvas.setColor(CellInactiveColor);

  // Draw cell background (slightly inset)
  canvas.fill(kPad, kPad, w - kPad * 2.0f, h - kPad * 2.0f);

  // Flash overlay: draw lighter color when flashing
  if (flashT > 0.0f)
  {
    canvas.setColor(CellPlayheadColor);
    canvas.fill(kPad, kPad, w - kPad * 2.0f, h - kPad * 2.0f);
  }

  // Velocity bar (thin strip at bottom of active cells)
  if (mActive && mVelocity < 0.99f)
  {
    canvas.setColor(KnobTrackColor);
    float barH = 3.0f;
    float barY = h - kPad - barH;
    float barW = (w - kPad * 2.0f) * mVelocity;
    canvas.fill(kPad, barY, barW, barH);
  }

  // Probability indicator (small dot in top-right corner for prob < 1)
  if (mActive && mProbability < 0.99f)
  {
    float dotR = 3.0f;
    canvas.setColor(CellAccentColor);
    canvas.circle(w - kPad - dotR * 2.0f, kPad, dotR * 2.0f);
  }
}

void StepCell::mouseDown(const visage::MouseEvent& e)
{
  mIsDragging       = true;
  mDragStartY       = e.y;
  mDragStartVelocity = mVelocity;
}

void StepCell::mouseDrag(const visage::MouseEvent& e)
{
  if (!mIsDragging)
    return;

  int dy = mDragStartY - e.y;
  if (std::abs(dy) > 3)
  {
    // Vertical drag adjusts velocity
    float newVel = mDragStartVelocity + static_cast<float>(dy) * 0.01f;
    newVel = std::max(0.0f, std::min(1.0f, newVel));
    if (newVel != mVelocity)
    {
      mVelocity = newVel;
      mOnVelocityChange.call(mVelocity);
      redraw();
    }
  }
}

void StepCell::mouseUp(const visage::MouseEvent& e)
{
  if (mIsDragging)
  {
    mIsDragging = false;
    int dy = mDragStartY - e.y;
    if (std::abs(dy) <= 3)
    {
      // Small movement = toggle
      mActive = !mActive;
      mOnToggle.call(mActive);
      redraw();
    }
  }
}

void StepCell::setActive(bool active)
{
  if (mActive != active)
  {
    mActive = active;
    redraw();
  }
}

void StepCell::setVelocity(float velocity)
{
  float v = std::max(0.0f, std::min(1.0f, velocity));
  if (mVelocity != v)
  {
    mVelocity = v;
    redraw();
  }
}

void StepCell::setProbability(float probability)
{
  float p = std::max(0.0f, std::min(1.0f, probability));
  if (mProbability != p)
  {
    mProbability = p;
    redraw();
  }
}

void StepCell::setPlayhead(bool isPlayhead)
{
  if (mIsPlayhead != isPlayhead)
  {
    mIsPlayhead = isPlayhead;
    redraw();
  }
}

void StepCell::triggerFlash()
{
  mFlashTimer = kFlashDurationMs;
  redraw();
}

bool StepCell::tickFlash(float deltaMs)
{
  if (mFlashTimer <= 0.0f)
    return false;
  mFlashTimer = std::max(0.0f, mFlashTimer - deltaMs);
  redraw();
  return mFlashTimer > 0.0f;
}
