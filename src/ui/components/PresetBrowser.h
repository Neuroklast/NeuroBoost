#pragma once

#include "visage_ui/frame.h"
#include "visage_graphics/canvas.h"
#include <functional>
#include <string>
#include <vector>

/// Horizontal preset browser bar.
/// Layout:  [<]  [Preset Name Display (*)]  [>]
/// - Left/right arrows cycle through presets.
/// - The centre label shows the current name; if mIsDirty, a '*' suffix is shown.
/// - Fires mOnPresetSelected(int index) when the selection changes.
class PresetBrowser : public visage::Frame
{
public:
  PresetBrowser();

  void draw(visage::Canvas& canvas) override;
  void mouseDown(const visage::MouseEvent& e) override;

  // -----------------------------------------------------------------------
  // Configuration
  // -----------------------------------------------------------------------

  /// Provide the list of preset names to display.
  void setPresetNames(std::vector<std::string> names);

  /// Set the currently displayed preset index (0-based).
  void setCurrentPreset(int index);
  int  getCurrentPreset() const { return mCurrentIndex; }

  /// Set the dirty / modified state (shows '*' suffix when true).
  void setDirty(bool dirty);
  bool isDirty() const { return mIsDirty; }

  // -----------------------------------------------------------------------
  // Callback: called when the user selects a different preset
  // -----------------------------------------------------------------------
  std::function<void(int)> mOnPresetSelected;

private:
  std::vector<std::string> mNames;
  int  mCurrentIndex = 0;
  bool mIsDirty      = false;

  // Hit regions for arrow buttons (computed in draw)
  float mPrevBtnX = 0.0f, mPrevBtnW = 0.0f;
  float mNextBtnX = 0.0f, mNextBtnW = 0.0f;

  void selectPreset(int index);
  void drawArrowButton(visage::Canvas& canvas,
                       const char* label,
                       float x, float y, float w, float h,
                       bool hovered);
};
