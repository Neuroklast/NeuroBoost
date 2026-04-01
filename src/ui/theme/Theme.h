#pragma once
// NeuroBoost Theme — Centralized color definitions
// All VISAGE_THEME_COLOR declarations for the NeuroBoost UI.

#ifdef IPLUG_EDITOR
#include "visage_graphics/font.h"
#include <string>

// Create a Font from the bundled Lato typeface.
// NEUROBOOST_FONT_PATH is defined in CMakeLists.txt when Visage is available.
// If undefined, returns an empty Font (text won't render, but won't crash).
inline visage::Font makeFont(float size) {
#ifdef NEUROBOOST_FONT_PATH
  static const std::string kFontPath = NEUROBOOST_FONT_PATH;
  return visage::Font(size, kFontPath);
#else
  #pragma message("NEUROBOOST_FONT_PATH not defined — UI text will not render.")
  return visage::Font();
#endif
}
#endif

// Background
VISAGE_THEME_COLOR(BackgroundColor,   0xff0d0d1a);  // Dark space background
VISAGE_THEME_COLOR(PanelColor,        0xff1a1a2e);  // Panel background
VISAGE_THEME_COLOR(CellInactiveColor, 0xff16213e);  // Inactive step cell
VISAGE_THEME_COLOR(CellActiveColor,   0xff4a9eff);  // Active step cell
VISAGE_THEME_COLOR(CellPlayheadColor, 0xff00d2ff);  // Currently playing step
VISAGE_THEME_COLOR(CellAccentColor,   0xffff6b35);  // Accented step
VISAGE_THEME_COLOR(CellProbabilityColor, 0xff00ccaa); // Probability bar (teal)

// Knobs & Controls
VISAGE_THEME_COLOR(KnobBackgroundColor, 0xff16213e);
VISAGE_THEME_COLOR(KnobArcColor,        0xff4a9eff);
VISAGE_THEME_COLOR(KnobTrackColor,      0xff555555);

// Text
VISAGE_THEME_COLOR(TextColor,    0xffffffff);
VISAGE_THEME_COLOR(TextDimColor, 0xff888888);
VISAGE_THEME_COLOR(AccentColor,  0xff00d2ff);

// Transport
VISAGE_THEME_COLOR(PlayColor, 0xff00ff88);
VISAGE_THEME_COLOR(StopColor, 0xffff4444);

// Header / Selector
VISAGE_THEME_COLOR(SelectorBgColor,     0xff222244);
VISAGE_THEME_COLOR(SelectorBorderColor, 0xff4a9eff);
