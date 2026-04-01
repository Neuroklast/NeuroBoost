#include "ModeSelector.h"
#include "../theme/Theme.h"
#include <algorithm>

ModeSelector::ModeSelector()
{
}

void ModeSelector::draw(visage::Canvas& canvas)
{
  float w = width();
  float h = height();

  // Background
  canvas.setColor(SelectorBgColor);
  canvas.fill(0.0f, 0.0f, w, h);

  // Border (top and bottom lines)
  canvas.setColor(SelectorBorderColor);
  canvas.fill(0.0f, 0.0f, w, 1.0f);
  canvas.fill(0.0f, h - 1.0f, w, 1.0f);

  // Current selection label
  const std::string& label = getSelectedLabel();
  visage::Font font = makeFont(12.0f);
  canvas.setColor(TextColor);
  canvas.text(label.c_str(), font, visage::Font::kCenter,
              4.0f, 0.0f, w - 16.0f, h);

  // Inline dropdown list (when open)
  if (mIsOpen && !mOptions.empty())
  {
    float itemH = std::max(h, 20.0f);
    float listH = itemH * static_cast<float>(mOptions.size());

    canvas.setColor(PanelColor);
    canvas.fill(0.0f, h, w, listH);

    for (int i = 0; i < static_cast<int>(mOptions.size()); ++i)
    {
      float itemY = h + itemH * static_cast<float>(i);

      if (i == mSelectedIndex)
      {
        canvas.setColor(CellActiveColor);
        canvas.fill(1.0f, itemY + 1.0f, w - 2.0f, itemH - 2.0f);
      }

      canvas.setColor(i == mSelectedIndex ? TextColor : TextDimColor);
      canvas.text(mOptions[i].c_str(), font, visage::Font::kCenter,
                  8.0f, itemY, w - 16.0f, itemH);
    }

    // Border around list
    canvas.setColor(SelectorBorderColor);
    canvas.fill(0.0f, h, w, 1.0f);
    canvas.fill(0.0f, h + listH - 1.0f, w, 1.0f);
  }
}

void ModeSelector::mouseDown(const visage::MouseEvent& e)
{
  if (!mIsOpen)
  {
    mIsOpen = true;
    redraw();
    return;
  }

  // If open, check if click is on an item
  float h     = height();
  float itemH = std::max(h, 20.0f);

  if (e.position.y > h && !mOptions.empty())
  {
    int idx = static_cast<int>((e.position.y - h) / itemH);
    if (idx >= 0 && idx < static_cast<int>(mOptions.size()))
    {
      selectIndex(idx);
    }
  }

  mIsOpen = false;
  redraw();
}

bool ModeSelector::mouseWheel(const visage::MouseEvent& e)
{
  if (mOptions.empty())
    return false;

  int next = mSelectedIndex + (e.wheel_delta_y > 0.0f ? 1 : -1);
  next = std::max(0, std::min(static_cast<int>(mOptions.size()) - 1, next));
  if (next != mSelectedIndex)
    selectIndex(next);
  return true;
}

void ModeSelector::setOptions(std::vector<std::string> options)
{
  mOptions = std::move(options);
  if (mSelectedIndex >= static_cast<int>(mOptions.size()))
    mSelectedIndex = 0;
  redraw();
}

void ModeSelector::setSelectedIndex(int index)
{
  if (index >= 0 && index < static_cast<int>(mOptions.size()))
  {
    mSelectedIndex = index;
    redraw();
  }
}

const std::string& ModeSelector::getSelectedLabel() const
{
  static const std::string kEmpty;
  if (mOptions.empty())
    return kEmpty;
  int i = std::max(0, std::min(mSelectedIndex, static_cast<int>(mOptions.size()) - 1));
  return mOptions[i];
}

void ModeSelector::selectIndex(int index)
{
  if (index == mSelectedIndex)
    return;
  mSelectedIndex = index;
  mOnSelectionChanged.callback(mSelectedIndex);
  redraw();
}
