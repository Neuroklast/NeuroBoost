// Algorithms.cpp – implementation of all NeuroBoost generation algorithms.
// Realtime-safe: no heap allocations, no exceptions, no I/O, no std::string.

#include "Algorithms.h"

#include <cmath>   // std::sqrt, std::pow
#include <cstring> // std::memcpy, std::strlen

// =============================================================================
// A1: Fibonacci rhythm generator
// =============================================================================

void Algorithms::generateFibonacci(int stepCount, bool* output, int outputSize)
{
  if (outputSize <= 0 || output == nullptr)
    return;

  int n = (stepCount < outputSize) ? stepCount : outputSize;

  for (int i = 0; i < n; ++i)
    output[i] = false;

  if (stepCount <= 0)
    return;

  // Generate Fibonacci numbers and mark positions (fib % stepCount) as active.
  // Cap the loop to prevent running indefinitely; Fibonacci grows exponentially
  // so we will have covered all residues long before MAX_STEPS * 16 iterations.
  int a = 1, b = 1;
  for (int iter = 0; iter < MAX_STEPS * 16; ++iter)
  {
    int pos = ((a - 1) % stepCount + stepCount) % stepCount; // 1-indexed → 0-indexed
    if (pos < n)
      output[pos] = true;

    int next = a + b;
    // Prevent integer overflow: once numbers exceed a safe bound, wrap with
    // safe-modulo so the position calculation stays correct.
    if (next > 1000000)
    {
      // Reduce both terms modulo stepCount to keep arithmetic from overflowing
      a = ((a % stepCount) + stepCount) % stepCount;
      b = ((b % stepCount) + stepCount) % stepCount;
      next = a + b;
    }
    a = b;
    b = next;

    // Stop once a is 0 (all residues have been visited when both wrap to 0)
    if (a == 0 && b == 0)
      break;
  }
}

// =============================================================================
// A2: L-System pattern generator
// =============================================================================

void Algorithms::generateLSystem(char axiom, const char* ruleA, const char* ruleB,
                                  int iterations, bool* output, int outputSize)
{
  if (outputSize <= 0 || output == nullptr)
    return;

  for (int i = 0; i < outputSize; ++i)
    output[i] = false;

  if (ruleA == nullptr || ruleB == nullptr)
    return;

  int lenA = 0;
  while (ruleA[lenA] != '\0' && lenA < MAX_LSYSTEM_LENGTH) ++lenA;
  int lenB = 0;
  while (ruleB[lenB] != '\0' && lenB < MAX_LSYSTEM_LENGTH) ++lenB;

  // Double-buffer: read from buf[readBuf], write to buf[writeBuf]
  // NOTE: static buffers — call only from the control thread (never from processBlock).
  // Single-instance use is safe because regeneratePattern() is serialised by design.
  static char buf[2][MAX_LSYSTEM_LENGTH];
  int readBuf  = 0;
  int writeBuf = 1;

  // Initialise with axiom
  buf[readBuf][0] = axiom;
  int currentLen  = 1;

  for (int iter = 0; iter < iterations; ++iter)
  {
    int writeLen = 0;

    for (int i = 0; i < currentLen; ++i)
    {
      char c = buf[readBuf][i];
      const char* rule = nullptr;
      int ruleLen      = 0;

      if (c == 'A')      { rule = ruleA; ruleLen = lenA; }
      else if (c == 'B') { rule = ruleB; ruleLen = lenB; }

      if (rule != nullptr && ruleLen > 0)
      {
        for (int r = 0; r < ruleLen; ++r)
        {
          if (writeLen >= MAX_LSYSTEM_LENGTH)
            break; // truncate
          buf[writeBuf][writeLen++] = rule[r];
        }
      }
      else
      {
        if (writeLen < MAX_LSYSTEM_LENGTH)
          buf[writeBuf][writeLen++] = c;
      }

      if (writeLen >= MAX_LSYSTEM_LENGTH)
        break;
    }

    // Swap buffers
    int tmp   = readBuf;
    readBuf   = writeBuf;
    writeBuf  = tmp;
    currentLen = writeLen;
  }

  // Map result: 'A' → true, everything else → false
  int limit = (currentLen < outputSize) ? currentLen : outputSize;
  for (int i = 0; i < limit; ++i)
    output[i] = (buf[readBuf][i] == 'A');
}

// =============================================================================
// A3: Cellular Automata (Wolfram 1D)
// =============================================================================

void Algorithms::generateCellularAutomata(int rule, int steps, int iterations,
                                           bool* output, int outputSize)
{
  if (outputSize <= 0 || output == nullptr)
    return;

  for (int i = 0; i < outputSize; ++i)
    output[i] = false;

  if (steps <= 0)
    return;

  // Clamp dimensions
  int width  = (steps < MAX_CA_WIDTH)  ? steps : MAX_CA_WIDTH;
  int height = (iterations + 1 < MAX_CA_HEIGHT) ? (iterations + 1) : MAX_CA_HEIGHT;
  int gens   = height - 1; // actual generations to evolve

  // Fixed-size grid
  // NOTE: static storage — call only from the control thread (never from processBlock).
  // Single-instance use is safe because regeneratePattern() is serialised by design.
  static bool grid[MAX_CA_HEIGHT][MAX_CA_WIDTH];
  for (int r = 0; r < MAX_CA_HEIGHT; ++r)
    for (int c = 0; c < MAX_CA_WIDTH; ++c)
      grid[r][c] = false;

  // Initialise first row: single cell in the centre
  grid[0][width / 2] = true;

  // Evolve
  for (int gen = 0; gen < gens; ++gen)
  {
    for (int col = 0; col < width; ++col)
    {
      int left   = ((col - 1) % width + width) % width;
      int right  = (col + 1) % width;
      int idx    = (grid[gen][left]  ? 4 : 0)
                 | (grid[gen][col]   ? 2 : 0)
                 | (grid[gen][right] ? 1 : 0);
      grid[gen + 1][col] = ((rule >> idx) & 1) != 0;
    }
  }

  // Bottom row → output
  int lastRow = gens;
  int limit   = (width < outputSize) ? width : outputSize;
  for (int col = 0; col < limit; ++col)
    output[col] = grid[lastRow][col];
}

// =============================================================================
// A4: Fractal mapping (Mandelbrot set)
// =============================================================================

void Algorithms::generateFractal(int steps, double cx, double cy, double zoom,
                                  int maxIter, int threshold,
                                  int* iterationCounts, bool* output, int outputSize)
{
  if (outputSize <= 0 || output == nullptr || iterationCounts == nullptr)
    return;

  for (int i = 0; i < outputSize; ++i)
  {
    output[i]          = false;
    iterationCounts[i] = 0;
  }

  if (steps <= 0)
    return;

  if (maxIter <= 0) maxIter = 1;
  if (maxIter > MAX_FRACTAL_ITERATIONS) maxIter = MAX_FRACTAL_ITERATIONS;

  int n = (steps < outputSize) ? steps : outputSize;

  for (int i = 0; i < n; ++i)
  {
    // Sample a point along a horizontal line through (cx, cy)
    double real = (zoom > 0.0)
                    ? cx - zoom * 0.5 + zoom * static_cast<double>(i) / static_cast<double>(steps)
                    : cx;
    double imag = cy;

    // Mandelbrot iteration: z = z² + c
    double zr = 0.0, zi = 0.0;
    int iter = 0;
    while (iter < maxIter && (zr * zr + zi * zi) <= 4.0)
    {
      double zr2 = zr * zr - zi * zi + real;
      double zi2 = 2.0 * zr * zi + imag;
      zr = zr2;
      zi = zi2;
      ++iter;
    }

    iterationCounts[i] = iter;
    output[i]          = (iter > threshold);
  }
}

// =============================================================================
// A5: Markov chain pitch sequence generator
// =============================================================================

void Algorithms::generateMarkov(const double transitionMatrix[12][12],
                                 int startNote, int steps,
                                 std::mt19937& rng,
                                 int* output, int outputSize)
{
  if (outputSize <= 0 || output == nullptr)
    return;

  for (int i = 0; i < outputSize; ++i)
    output[i] = 0;

  if (steps <= 0)
    return;

  int n = (steps < outputSize) ? steps : outputSize;

  // Clamp startNote to [0, 11]
  int current = ((startNote % 12) + 12) % 12;

  std::uniform_real_distribution<double> dist(0.0, 1.0);

  for (int i = 0; i < n; ++i)
  {
    output[i] = current;

    // Build cumulative distribution from the current row
    double rowSum = 0.0;
    for (int j = 0; j < 12; ++j)
      rowSum += transitionMatrix[current][j];

    double r = dist(rng);
    int next = current; // fallback: stay on same note

    if (rowSum <= 0.0)
    {
      // Absorbing state → uniform fallback
      next = static_cast<int>(r * 12.0);
      if (next >= 12) next = 11;
    }
    else
    {
      double cumulative = 0.0;
      bool found = false;
      for (int j = 0; j < 12; ++j)
      {
        cumulative += transitionMatrix[current][j] / rowSum;
        if (r <= cumulative)
        {
          next  = j;
          found = true;
          break;
        }
      }
      if (!found)
        next = 11; // numerical rounding fallback
    }

    current = next;
  }
}

// =============================================================================
// A6: Probability / weighted random
// =============================================================================

void Algorithms::generateProbability(const double* probabilities, int steps,
                                      std::mt19937& rng,
                                      bool* output, int outputSize)
{
  if (outputSize <= 0 || output == nullptr || probabilities == nullptr)
    return;

  for (int i = 0; i < outputSize; ++i)
    output[i] = false;

  if (steps <= 0)
    return;

  int n = (steps < outputSize) ? steps : outputSize;
  std::uniform_real_distribution<double> dist(0.0, 1.0);

  for (int i = 0; i < n; ++i)
    output[i] = (dist(rng) < probabilities[i]);
}

// -----------------------------------------------------------------------------
// constexpr definitions — required in exactly one translation unit (C++14/17)
// -----------------------------------------------------------------------------

constexpr double Algorithms::MARKOV_PRESET_BLUES[12][12];
constexpr double Algorithms::MARKOV_PRESET_JAZZ[12][12];
constexpr double Algorithms::MARKOV_PRESET_MINIMAL[12][12];

