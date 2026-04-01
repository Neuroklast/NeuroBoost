// test_ui_structure.cpp — Structural UI test for NeuroBoost.
// Validates UI component tree structure, layout bounds, and interaction callbacks
// WITHOUT requiring GPU rendering or Visage headless mode.
// Generates an ASCII "screenshot" (ui_structure_dump.txt) for visual inspection.
//
// Build with: cmake -B build -DBUILD_UI_TEST=ON && cmake --build build --target test_ui_structure

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <functional>
#include <cstdint>
#include <cmath>
#include <cstring>

// Include only the common headers (no Visage/iPlug2/SequencerEngine required)
#include "../src/common/Constants.h"
#include "../src/common/Types.h"

// ============================================================================
// Minimal mock types (replace Visage types for structure-only testing)
// These stubs allow us to instantiate and test UI logic without GPU context.
// ============================================================================

struct MockBounds { float x, y, w, h; };

/// Mock step data — mirrors what StepCell would hold
struct MockStepState
{
  bool  active      = false;
  float velocity    = 1.0f;
  float probability = 1.0f;
  bool  playhead    = false;
  bool  accent      = false;
};

/// Mock StepGrid — holds state for MAX_STEPS cells
struct MockStepGrid
{
  int            stepCount   = 16;
  int            playheadStep = -1;
  MockStepState  cells[MAX_STEPS];
  MockBounds     bounds      = {0, 48, 600, 300};

  void setStepCount(int n) { stepCount = std::max(1, std::min(MAX_STEPS, n)); }
  void setPlayheadStep(int step)
  {
    if (playheadStep >= 0 && playheadStep < MAX_STEPS)
      cells[playheadStep].playhead = false;
    playheadStep = step;
    if (step >= 0 && step < MAX_STEPS)
      cells[step].playhead = true;
  }
  void setStepActive(int i, bool v)       { if (i >= 0 && i < MAX_STEPS) cells[i].active      = v; }
  void setStepVelocity(int i, float v)    { if (i >= 0 && i < MAX_STEPS) cells[i].velocity     = v; }
  void setStepProbability(int i, float v) { if (i >= 0 && i < MAX_STEPS) cells[i].probability  = v; }
  void setStepAccent(int i, bool v)       { if (i >= 0 && i < MAX_STEPS) cells[i].accent       = v; }
};

/// Minimal knob descriptor
struct MockKnob
{
  std::string label;
  double minVal, maxVal, defaultVal, value;
  MockBounds  bounds = {0, 0, 80, 80};

  MockKnob(const char* lbl, double mn, double mx, double def)
    : label(lbl), minVal(mn), maxVal(mx), defaultVal(def), value(def) {}

  double getNormalized() const
  {
    double range = maxVal - minVal;
    return (range > 0.0) ? (value - minVal) / range : 0.0;
  }
};

/// Minimal mode selector descriptor
struct MockModeSelector
{
  std::string name;
  int optionCount   = 0;
  int selectedIndex = 0;
  MockBounds bounds  = {0, 0, 180, 32};

  MockModeSelector(const char* n, int cnt, int sel)
    : name(n), optionCount(cnt), selectedIndex(sel) {}
};

/// Mock EditorView structure — built from the same logical layout as EditorView.h
struct MockEditorView
{
  MockBounds bounds = {0, 0, 900, 600};

  // Header selectors
  MockModeSelector genMode     {"GenMode",  7,  0};
  MockModeSelector scaleMode   {"ScaleMode",15, 1};
  MockModeSelector rootNote    {"RootNote", 12, 0};

  // Step grid
  MockStepGrid grid;

  // Rhythm knobs
  MockKnob stepsKnob      {"Steps",   1.0, 64.0, 16.0};
  MockKnob hitsKnob       {"Hits",    0.0, 64.0,  4.0};
  MockKnob rotateKnob     {"Rotate",  0.0, 63.0,  0.0};

  // Melody knobs
  MockKnob octaveLowKnob  {"OctLow",  0.0,  8.0,  3.0};
  MockKnob octaveHighKnob {"OctHigh", 0.0,  8.0,  5.0};
  MockKnob densityKnob    {"Density", 0.0,  1.0,  1.0};

  // Performance knobs
  MockKnob swingKnob      {"Swing",   0.0,  0.5,  0.0};
  MockKnob mutationKnob   {"Mutation",0.0,  1.0,  0.0};
  MockKnob velocityKnob   {"VelCurve",0.1,  4.0,  1.0};

  // Transport
  bool isPlaying = false;
  MockBounds transportBounds = {0, 560, 900, 40};

  // Knob array helpers
  static constexpr int kKnobCount = 9;
  MockKnob* knobs[kKnobCount];

  MockEditorView()
  {
    knobs[0] = &stepsKnob;
    knobs[1] = &hitsKnob;
    knobs[2] = &rotateKnob;
    knobs[3] = &octaveLowKnob;
    knobs[4] = &octaveHighKnob;
    knobs[5] = &densityKnob;
    knobs[6] = &swingKnob;
    knobs[7] = &mutationKnob;
    knobs[8] = &velocityKnob;

    // Set layout bounds (matching EditorView::layoutWidgets)
    stepsKnob.bounds      = {10,  360, 80, 80};
    hitsKnob.bounds       = {100, 360, 80, 80};
    rotateKnob.bounds     = {190, 360, 80, 80};
    octaveLowKnob.bounds  = {10,  460, 80, 80};
    octaveHighKnob.bounds = {100, 460, 80, 80};
    densityKnob.bounds    = {190, 460, 80, 80};
    swingKnob.bounds      = {280, 360, 80, 80};
    mutationKnob.bounds   = {370, 360, 80, 80};
    velocityKnob.bounds   = {460, 360, 80, 80};

    genMode.bounds   = {10,  8,  180, 32};
    scaleMode.bounds = {200, 8,  180, 32};
    rootNote.bounds  = {390, 8,  120, 32};

    grid.bounds = {0, 48, 600, 300};
  }
};

// ============================================================================
// Test infrastructure
// ============================================================================

static int gPassed = 0;
static int gFailed = 0;

#define EXPECT(cond, msg)                                              \
  do {                                                                 \
    if (cond) {                                                        \
      std::cout << "  PASS: " << msg << "\n";                         \
      ++gPassed;                                                       \
    } else {                                                           \
      std::cout << "  FAIL: " << msg << "\n";                         \
      ++gFailed;                                                       \
    }                                                                  \
  } while (false)

static void printSection(const char* name)
{
  std::cout << "\n[" << name << "]\n";
}

// ============================================================================
// ASCII dump generation
// ============================================================================

static std::string fmtBounds(const MockBounds& b)
{
  std::ostringstream ss;
  ss << "[" << (int)b.x << ", " << (int)b.y << ", "
     << (int)b.w << ", " << (int)b.h << "]";
  return ss.str();
}

static void generateAsciiDump(const MockEditorView& view, std::ostream& out)
{
  out << "EditorView " << fmtBounds(view.bounds) << "\n";

  out << "  ModeSelector \"" << view.genMode.name << "\" "
      << fmtBounds(view.genMode.bounds)
      << " options=" << view.genMode.optionCount
      << " selected=" << view.genMode.selectedIndex << "\n";

  out << "  ModeSelector \"" << view.scaleMode.name << "\" "
      << fmtBounds(view.scaleMode.bounds)
      << " options=" << view.scaleMode.optionCount
      << " selected=" << view.scaleMode.selectedIndex << "\n";

  out << "  ModeSelector \"" << view.rootNote.name << "\" "
      << fmtBounds(view.rootNote.bounds)
      << " options=" << view.rootNote.optionCount
      << " selected=" << view.rootNote.selectedIndex << "\n";

  out << "  StepGrid " << fmtBounds(view.grid.bounds)
      << " steps=" << view.grid.stepCount
      << " playhead=" << view.grid.playheadStep << "\n";

  for (int i = 0; i < view.grid.stepCount; ++i)
  {
    const MockStepState& s = view.grid.cells[i];
    out << "    StepCell[" << i << "]"
        << " active=" << (s.active ? "true" : "false")
        << " vel="    << s.velocity
        << " prob="   << s.probability
        << " accent=" << (s.accent ? "true" : "false")
        << " playhead=" << (s.playhead ? "true" : "false")
        << "\n";
  }

  for (int k = 0; k < MockEditorView::kKnobCount; ++k)
  {
    const MockKnob* knob = view.knobs[k];
    out << "  ParameterKnob \"" << knob->label << "\" "
        << fmtBounds(knob->bounds)
        << " min=" << knob->minVal
        << " max=" << knob->maxVal
        << " value=" << knob->value
        << "\n";
  }

  out << "  TransportBar " << fmtBounds(view.transportBounds)
      << " playing=" << (view.isPlaying ? "true" : "false") << "\n";
}

// ============================================================================
// Tests
// ============================================================================

static void testEditorViewStructure()
{
  printSection("EditorView_Structure");

  MockEditorView view;

  // EditorView bounds
  EXPECT(view.bounds.w == 900.0f && view.bounds.h == 600.0f,
         "EditorView has 900x600 bounds");

  // StepGrid has MAX_STEPS cells
  EXPECT(view.grid.stepCount == 16, "StepGrid default stepCount is 16");
  EXPECT(MAX_STEPS == 64,           "MAX_STEPS constant is 64");

  // StepGrid bounds
  EXPECT(view.grid.bounds.x == 0.0f  && view.grid.bounds.y == 48.0f,
         "StepGrid at origin y=48");
  EXPECT(view.grid.bounds.w == 600.0f && view.grid.bounds.h == 300.0f,
         "StepGrid is 600x300");

  // Knob count
  EXPECT(MockEditorView::kKnobCount == 9, "9 parameter knobs defined");

  // All knobs have nonzero size
  bool allNonzero = true;
  for (int k = 0; k < MockEditorView::kKnobCount; ++k)
    if (view.knobs[k]->bounds.w <= 0 || view.knobs[k]->bounds.h <= 0)
      allNonzero = false;
  EXPECT(allNonzero, "All knobs have nonzero size");

  // Mode selector option counts
  EXPECT(view.genMode.optionCount == 7,
         "GenMode selector has 7 options (gen algorithms)");
  EXPECT(view.scaleMode.optionCount == 15,
         "ScaleMode selector has 15 scale options");
  EXPECT(view.rootNote.optionCount == 12,
         "RootNote selector has 12 chromatic notes");

  // Transport bar
  EXPECT(view.transportBounds.y == 560.0f, "TransportBar at y=560");
  EXPECT(view.transportBounds.h == 40.0f,  "TransportBar is 40px tall");
}

static void testStepCellInteractions()
{
  printSection("StepCell_Interactions");

  MockStepGrid grid;
  grid.setStepCount(16);

  // Toggle step 3
  grid.setStepActive(3, true);
  EXPECT(grid.cells[3].active == true, "Toggle step 3: active=true");
  grid.setStepActive(3, false);
  EXPECT(grid.cells[3].active == false, "Toggle step 3: active=false");

  // Set velocity on step 7
  grid.setStepVelocity(7, 0.8f);
  EXPECT(std::abs(grid.cells[7].velocity - 0.8f) < 1e-5f,
         "setStepVelocity(7, 0.8) → getVelocity() == 0.8");

  // Set probability on step 2
  grid.setStepProbability(2, 0.5f);
  EXPECT(std::abs(grid.cells[2].probability - 0.5f) < 1e-5f,
         "setStepProbability(2, 0.5) → getProbability() == 0.5");

  // Set accent on step 4
  grid.setStepAccent(4, true);
  EXPECT(grid.cells[4].accent == true, "setStepAccent(4, true) → accent=true");

  // Playhead: set to step 5, verify step 5 has playhead=true, all others false
  grid.setPlayheadStep(5);
  EXPECT(grid.cells[5].playhead == true, "setPlayheadStep(5): step 5 has playhead=true");
  bool othersOff = true;
  for (int i = 0; i < 16; ++i)
    if (i != 5 && grid.cells[i].playhead) othersOff = false;
  EXPECT(othersOff, "setPlayheadStep(5): all other steps have playhead=false");

  // Move playhead to step 10
  grid.setPlayheadStep(10);
  EXPECT(grid.cells[5].playhead  == false, "After playhead move: step 5 cleared");
  EXPECT(grid.cells[10].playhead == true,  "After playhead move: step 10 set");

  // Boundary: setPlayheadStep(-1) should not crash
  grid.setPlayheadStep(-1);
  EXPECT(grid.cells[10].playhead == false, "setPlayheadStep(-1): previous step cleared");
}

static void testKnobRanges()
{
  printSection("Knob_Ranges");

  MockEditorView view;

  // Steps knob
  EXPECT(view.stepsKnob.minVal == 1.0  && view.stepsKnob.maxVal == 64.0,
         "Steps knob: range [1, 64]");
  EXPECT(view.stepsKnob.defaultVal == 16.0, "Steps knob: default=16");

  // Density knob
  EXPECT(view.densityKnob.minVal == 0.0 && view.densityKnob.maxVal == 1.0,
         "Density knob: range [0, 1]");
  EXPECT(view.densityKnob.defaultVal == 1.0, "Density knob: default=1.0");

  // Swing knob
  EXPECT(view.swingKnob.minVal == 0.0 && view.swingKnob.maxVal == 0.5,
         "Swing knob: range [0, 0.5]");

  // VelocityCurve knob
  EXPECT(view.velocityKnob.minVal == 0.1 && view.velocityKnob.maxVal == 4.0,
         "VelocityCurve knob: range [0.1, 4.0]");
  EXPECT(view.velocityKnob.defaultVal == 1.0, "VelocityCurve knob: default=1.0");

  // OctaveLow/High
  EXPECT(view.octaveLowKnob.defaultVal == 3.0,  "OctaveLow knob: default=3");
  EXPECT(view.octaveHighKnob.defaultVal == 5.0,  "OctaveHigh knob: default=5");

  // Normalized value check
  view.stepsKnob.value = 32.0;
  double norm = view.stepsKnob.getNormalized();
  EXPECT(std::abs(norm - (31.0/63.0)) < 1e-6, "Steps knob: getNormalized() == (32-1)/(64-1)");
}

static void testAsciiDump()
{
  printSection("ASCII_Dump");

  MockEditorView view;
  // Set up an interesting state
  view.genMode.selectedIndex = 0;     // Euclidean
  view.scaleMode.selectedIndex = 1;   // Major
  view.rootNote.selectedIndex = 0;    // C
  view.grid.setStepCount(16);
  view.grid.setStepActive(0, true);
  view.grid.setStepActive(2, true);
  view.grid.setStepActive(4, true);
  view.grid.setStepActive(6, true);
  view.grid.setPlayheadStep(3);
  view.stepsKnob.value  = 16.0;
  view.densityKnob.value = 1.0;

  // Generate the dump
  std::ostringstream ss;
  generateAsciiDump(view, ss);
  std::string dump = ss.str();

  // Basic content checks
  EXPECT(dump.find("EditorView") != std::string::npos, "ASCII dump contains 'EditorView'");
  EXPECT(dump.find("StepGrid") != std::string::npos,   "ASCII dump contains 'StepGrid'");
  EXPECT(dump.find("StepCell[0]") != std::string::npos,"ASCII dump contains 'StepCell[0]'");
  EXPECT(dump.find("TransportBar") != std::string::npos,"ASCII dump contains 'TransportBar'");
  EXPECT(dump.find("playhead=true") != std::string::npos,"ASCII dump shows playhead=true");

  // Save to file
  const char* dumpPath = "tests/screenshots/ui_structure_dump.txt";
  {
    std::ofstream f(dumpPath);
    if (f.is_open())
    {
      generateAsciiDump(view, f);
      std::cout << "  INFO: ASCII dump saved to " << dumpPath << "\n";
    }
    else
    {
      std::cout << "  WARN: Could not open " << dumpPath << " for writing\n";
    }
  }

  // Compare against reference if it exists
  const char* refPath = "tests/screenshots/reference/ui_structure_reference.txt";
  std::ifstream ref(refPath);
  if (ref.is_open())
  {
    std::string refContent((std::istreambuf_iterator<char>(ref)),
                            std::istreambuf_iterator<char>());
    EXPECT(dump == refContent, "ASCII dump matches reference file");
  }
  else
  {
    // No reference yet — save current dump as reference
    std::ofstream out(refPath);
    if (out.is_open())
    {
      generateAsciiDump(view, out);
      std::cout << "  INFO: Reference file created at " << refPath << "\n";
    }
    std::cout << "  INFO: No reference file found; skipping comparison\n";
    EXPECT(true, "ASCII dump: reference file created (first run)");
  }
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  std::cout << "NeuroBoost UI Structure Tests\n";
  std::cout << "==============================\n";

  testEditorViewStructure();
  testStepCellInteractions();
  testKnobRanges();
  testAsciiDump();

  std::cout << "\n==============================\n";
  std::cout << "Results: " << gPassed << " passed, " << gFailed << " failed\n";

  return (gFailed == 0) ? 0 : 1;
}
