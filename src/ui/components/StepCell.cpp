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

  // Determine cell color based on state
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

  // Accent border: orange outline
  if (mAccent)
  {
    canvas.setColor(CellAccentColor);
    canvas.fill(kPad, kPad, w - kPad * 2.0f, 2.0f);               // top
    canvas.fill(kPad, h - kPad - 2.0f, w - kPad * 2.0f, 2.0f);    // bottom
    canvas.fill(kPad, kPad, 2.0f, h - kPad * 2.0f);               // left
    canvas.fill(w - kPad - 2.0f, kPad, 2.0f, h - kPad * 2.0f);    // right
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

  // Probability bar (distinct teal/blue overlay at top for prob < 1)
  if (mActive && mProbability < 0.99f)
  {
    canvas.setColor(CellProbabilityColor);
    float barH = 3.0f;
    float barW = (w - kPad * 2.0f) * mProbability;
    canvas.fill(kPad, kPad, barW, barH);
  }
}

void StepCell::mouseDown(const visage::MouseEvent& e)
{
  if (e.isRightButton())
  {
    // Right-click toggles accent mode
    mAccent = !mAccent;
    mOnAccentToggle.callback(mAccent);
    redraw();
    return;
  }

  mIsDragging         = true;
  mDragStartY         = static_cast<int>(e.position.y);
  mDragStartVelocity  = mVelocity;
  mDragStartProbability = mProbability;
  mIsShiftDrag        = e.isShiftDown();
}

void StepCell::mouseDrag(const visage::MouseEvent& e)
{
  if (!mIsDragging)
    return;

  int dy = mDragStartY - static_cast<int>(e.position.y);
  if (std::abs(dy) > 3)
  {
    if (mIsShiftDrag)
    {
      // Shift+drag adjusts probability
      float newProb = mDragStartProbability + static_cast<float>(dy) * 0.01f;
      newProb = std::max(0.0f, std::min(1.0f, newProb));
      if (newProb != mProbability)
      {
        mProbability = newProb;
        mOnProbabilityChange.callback(mProbability);
        redraw();
      }
    }
    else
    {
      // Normal drag adjusts velocity
      float newVel = mDragStartVelocity + static_cast<float>(dy) * 0.01f;
      newVel = std::max(0.0f, std::min(1.0f, newVel));
      if (newVel != mVelocity)
      {
        mVelocity = newVel;
        mOnVelocityChange.callback(mVelocity);
        redraw();
      }
    }
  }
}

void StepCell::mouseUp(const visage::MouseEvent& e)
{
  if (mIsDragging)
  {
    mIsDragging = false;
    int dy = mDragStartY - static_cast<int>(e.position.y);
    if (std::abs(dy) <= 3)
    {
      // Small movement = toggle
      mActive = !mActive;
      mOnToggle.callback(mActive);
      redraw();
    }
  }
}

void StepCell::setAccent(bool accent)
{
  if (mAccent != accent)
  {
    mAccent = accent;
    redraw();
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
