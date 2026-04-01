#include "StepGrid.h"
#include "../theme/Theme.h"
#include <algorithm>
#include <cmath>

StepGrid::StepGrid()
{
  for (int i = 0; i < MAX_STEPS; ++i)
  {
    addChild(&mCells[i]);

    int idx = i;  // capture by value
    mCells[i].onToggle() = [this, idx](bool active) {
      mOnStepToggled.callback(idx, active);
    };

    mCells[i].onVelocityChange() = [this, idx](float vel) {
      mOnVelocityChanged.callback(idx, vel);
    };

    mCells[i].onProbabilityChange() = [this, idx](float prob) {
      mOnProbabilityChanged.callback(idx, prob);
    };

    mCells[i].onAccentToggle() = [this, idx](bool accent) {
      mOnAccentToggled.callback(idx, accent);
    };
  }
}

void StepGrid::draw(visage::Canvas& /*canvas*/)
{
  // Background is drawn by parent; cells draw themselves.
}

void StepGrid::resized()
{
  layoutCells();
}

void StepGrid::setStepCount(int count)
{
  mStepCount = std::max(1, std::min(MAX_STEPS, count));

  // Show only cells within the active count
  for (int i = 0; i < MAX_STEPS; ++i)
    mCells[i].setVisible(i < mStepCount);

  layoutCells();
}

void StepGrid::setPlayheadStep(int step)
{
  if (step == mPlayheadStep)
    return;

  int prev = mPlayheadStep;
  mPlayheadStep = step;

  if (prev >= 0 && prev < MAX_STEPS)
    mCells[prev].setPlayhead(false);
  if (step >= 0 && step < MAX_STEPS)
    mCells[step].setPlayhead(true);
}

void StepGrid::setStepActive(int step, bool active)
{
  if (step >= 0 && step < MAX_STEPS)
    mCells[step].setActive(active);
}

void StepGrid::setStepVelocity(int step, float velocity)
{
  if (step >= 0 && step < MAX_STEPS)
    mCells[step].setVelocity(velocity);
}

void StepGrid::setStepProbability(int step, float probability)
{
  if (step >= 0 && step < MAX_STEPS)
    mCells[step].setProbability(probability);
}

void StepGrid::triggerStepFlash(int step)
{
  if (step >= 0 && step < MAX_STEPS)
    mCells[step].triggerFlash();
}

void StepGrid::layoutCells()
{
  if (mStepCount <= 0 || width() <= 0 || height() <= 0)
    return;

  int rows = static_cast<int>(std::ceil(static_cast<float>(mStepCount) / kColumns));
  float cellW = width()  / static_cast<float>(kColumns);
  float cellH = height() / static_cast<float>(rows);

  for (int i = 0; i < mStepCount; ++i)
  {
    int col = i % kColumns;
    int row = i / kColumns;
    mCells[i].setBounds(col * cellW, row * cellH, cellW, cellH);
  }

  // Hide unused cells
  for (int i = mStepCount; i < MAX_STEPS; ++i)
    mCells[i].setVisible(false);
}
