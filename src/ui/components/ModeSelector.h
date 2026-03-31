#pragma once

#include "visage_ui/frame.h"
#include "visage_graphics/canvas.h"
#include <vector>
#include <string>

/// Drop-down style selector for algorithm / scale / root-note parameters.
/// Renders as a flat button showing the current selection; clicking opens
/// an inline list of options.
class ModeSelector : public visage::Frame
{
public:
  ModeSelector();

  void draw(visage::Canvas& canvas) override;
  void mouseDown(const visage::MouseEvent& e) override;
  void mouseWheelMove(const visage::MouseEvent& e, float deltaY) override;

  // -----------------------------------------------------------------------
  // Configuration
  // -----------------------------------------------------------------------

  void setOptions(std::vector<std::string> options);
  void setSelectedIndex(int index);
  int  getSelectedIndex() const { return mSelectedIndex; }
  const std::string& getSelectedLabel() const;

  // -----------------------------------------------------------------------
  // Callback
  // -----------------------------------------------------------------------

  /// Fired when selection changes: void(int index)
  auto& onSelectionChanged() { return mOnSelectionChanged; }

private:
  std::vector<std::string> mOptions;
  int  mSelectedIndex = 0;
  bool mIsOpen        = false;

  visage::CallbackList<void(int)> mOnSelectionChanged;

  void selectIndex(int index);
};
