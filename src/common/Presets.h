#pragma once
#include "Types.h"

struct PresetData {
  const char*    name;
  GenerationMode mode;
  int            stepCount;
  ScaleMode      scale;
  int            rootNote;
  double         density;
  double         swing;
  int            euclideanHits;
  int            euclideanRotation;
  int            caRule;
  int            caIterations;
  double         fractalCx;
  double         fractalCy;
  double         fractalZoom;
  int            fractalMaxIter;
  int            fractalThreshold;
  int            markovPreset;
};

static constexpr int kNumFactoryPresets = 10;

constexpr PresetData FACTORY_PRESETS[kNumFactoryPresets] = {
  {"Classic 4/4",        GenerationMode::Euclidean,       16, ScaleMode::Major,          60, 1.0,  0.0,  4, 0,   0,  0, 0.0,  0.0, 0.0,   0,  0, 0},
  {"Euclidean 7/16",     GenerationMode::Euclidean,       16, ScaleMode::Minor,          60, 1.0,  0.0,  7, 0,   0,  0, 0.0,  0.0, 0.0,   0,  0, 0},
  {"Golden Ratio",       GenerationMode::Fibonacci,       16, ScaleMode::PentatonicMajor,60, 0.8,  0.0,  0, 0,   0,  0, 0.0,  0.0, 0.0,   0,  0, 0},
  {"Fractal Forest",     GenerationMode::LSystem,         32, ScaleMode::Dorian,         60, 0.9,  0.0,  0, 0,   0,  0, 0.0,  0.0, 0.0,   0,  0, 0},
  {"Rule 110",           GenerationMode::CellularAutomata,32, ScaleMode::Minor,          60, 1.0,  0.0,  0, 0, 110, 16, 0.0,  0.0, 0.0,   0,  0, 0},
  {"Mandelbrot Dance",   GenerationMode::Fractal,         16, ScaleMode::WholeTone,      60, 1.0,  0.0,  0, 0,   0,  0,-0.5,  0.0, 3.0, 100, 25, 0},
  {"Blues Walker",       GenerationMode::Markov,          16, ScaleMode::Blues,          60, 1.0,  0.0,  0, 0,   0,  0, 0.0,  0.0, 0.0,   0,  0, 0},
  {"Jazz Stroll",        GenerationMode::Markov,          16, ScaleMode::Dorian,         60, 0.85, 0.0,  0, 0,   0,  0, 0.0,  0.0, 0.0,   0,  0, 1},
  {"Sparse Probability", GenerationMode::Probability,     16, ScaleMode::PentatonicMinor,60, 0.4,  0.0,  0, 0,   0,  0, 0.0,  0.0, 0.0,   0,  0, 0},
  {"Euclidean Shuffle",  GenerationMode::Euclidean,       16, ScaleMode::Minor,          60, 1.0,  0.3,  5, 2,   0,  0, 0.0,  0.0, 0.0,   0,  0, 0},
};
