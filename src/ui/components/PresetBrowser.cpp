#include "PresetBrowser.h"
#include "../theme/Theme.h"

PresetBrowser::PresetBrowser()
{
}

void PresetBrowser::draw(visage::Canvas& canvas)
{
  const float w   = width();
  const float h   = height();
  const float bw  = 28.0f;  // arrow button width
  const float pad = 4.0f;

  // Background
  canvas.setColor(PresetBgColor);
  canvas.fill(0.0f, 0.0f, w, h);

  // Border line at bottom
  canvas.setColor(SelectorBorderColor);
  canvas.fill(0.0f, h - 1.0f, w, 1.0f);

  // Arrow buttons
  mPrevBtnX = pad;
  mPrevBtnW = bw;
  mNextBtnX = w - pad - bw;
  mNextBtnW = bw;

  drawArrowButton(canvas, "<", mPrevBtnX, pad, mPrevBtnW, h - 2.0f * pad, false);
  drawArrowButton(canvas, ">", mNextBtnX, pad, mNextBtnW, h - 2.0f * pad, false);

  // Preset name display
  std::string displayName = "---";
  if (!mNames.empty() && mCurrentIndex >= 0 &&
      mCurrentIndex < static_cast<int>(mNames.size()))
    displayName = mNames[static_cast<size_t>(mCurrentIndex)];

  if (mIsDirty)
    displayName += " *";

  visage::Font font = makeFont(12.0f);
  const float nameX = mPrevBtnX + mPrevBtnW + pad;
  const float nameW = mNextBtnX - nameX - pad;

  canvas.setColor(PresetTextColor);
  canvas.text(displayName.c_str(), font, visage::Font::kCenter,
              nameX, pad, nameW, h - 2.0f * pad);
}

void PresetBrowser::drawArrowButton(visage::Canvas& canvas,
                                    const char* label,
                                    float x, float y, float w, float h,
                                    bool /*hovered*/)
{
  canvas.setColor(SelectorBgColor);
  canvas.fill(x, y, w, h);

  canvas.setColor(SelectorBorderColor);
  canvas.fill(x,           y,           w,    1.0f);
  canvas.fill(x,           y + h - 1.0f, w,   1.0f);
  canvas.fill(x,           y,           1.0f, h);
  canvas.fill(x + w - 1.0f, y,          1.0f, h);

  visage::Font font = makeFont(12.0f);
  canvas.setColor(TextColor);
  canvas.text(label, font, visage::Font::kCenter, x, y, w, h);
}

void PresetBrowser::mouseDown(const visage::MouseEvent& e)
{
  const float h  = height();
  const float pad = 4.0f;

  auto hitTest = [&](float bx, float bw) {
    return e.position.x >= bx && e.position.x <= bx + bw &&
           e.position.y >= pad && e.position.y <= h - pad;
  };

  if (hitTest(mPrevBtnX, mPrevBtnW))
  {
    int next = mCurrentIndex - 1;
    if (next < 0) next = static_cast<int>(mNames.size()) - 1;
    selectPreset(next);
  }
  else if (hitTest(mNextBtnX, mNextBtnW))
  {
    int next = mCurrentIndex + 1;
    if (next >= static_cast<int>(mNames.size())) next = 0;
    selectPreset(next);
  }
}

void PresetBrowser::setPresetNames(std::vector<std::string> names)
{
  mNames = std::move(names);
  redraw();
}

void PresetBrowser::setCurrentPreset(int index)
{
  if (index < 0 || index >= static_cast<int>(mNames.size()))
    return;
  if (mCurrentIndex == index)
    return;
  mCurrentIndex = index;
  redraw();
}

void PresetBrowser::setDirty(bool dirty)
{
  if (mIsDirty != dirty)
  {
    mIsDirty = dirty;
    redraw();
  }
}

void PresetBrowser::selectPreset(int index)
{
  if (index < 0 || index >= static_cast<int>(mNames.size()))
    return;
  mCurrentIndex = index;
  mIsDirty = false;
  if (mOnPresetSelected)
    mOnPresetSelected(mCurrentIndex);
  redraw();
}
