#pragma once

#include "../common/Constants.h"

// DSP algorithm implementations.
// Realtime-safe: no heap allocations, no exceptions, no I/O.
class Algorithms
{
public:
  // Generate a Euclidean rhythm (Bjorklund algorithm) into a pre-allocated
  // boolean array. 'steps' active slots are distributed as evenly as possible
  // among 'total' steps, then rotated by 'rotation' positions.
  //
  // Parameters:
  //   total      - total number of steps (clamped to [0, outputSize])
  //   hits       - number of active beats (clamped to [0, total])
  //   rotation   - rotation offset (safe-modulo applied)
  //   output     - caller-supplied bool array, length >= outputSize
  //   outputSize - capacity of output[]
  //
  // The function writes exactly min(total, outputSize) elements.
  static void generateEuclidean(int total, int hits, int rotation,
                                bool* output, int outputSize)
  {
    if (outputSize <= 0 || output == nullptr)
      return;

    // Clamp
    if (total > outputSize) total = outputSize;
    if (total <= 0)
    {
      for (int i = 0; i < outputSize; ++i) output[i] = false;
      return;
    }

    if (hits <= 0)
    {
      for (int i = 0; i < total; ++i) output[i] = false;
      return;
    }

    if (hits >= total)
    {
      for (int i = 0; i < total; ++i) output[i] = true;
      return;
    }

    // Bjorklund / Bresenham distribution (iterative)
    // Use two fixed-size arrays; sizes bounded by MAX_STEPS.
    bool pattern[MAX_STEPS] = {};

    // pattern[i] = true if beat i carries a hit
    // Distribute hits as evenly as possible using Bresenham's line algorithm
    int remainder = total - hits;
    int perHit    = total / hits;
    int extra     = total % hits;

    int pos = 0;
    for (int h = 0; h < hits && pos < total; ++h)
    {
      pattern[pos] = true;
      pos += perHit;
      if (h < extra) ++pos;
    }

    // Apply rotation (safe modulo)
    int rot = ((rotation % total) + total) % total;
    for (int i = 0; i < total; ++i)
      output[i] = pattern[(i + rot) % total];
  }
};
