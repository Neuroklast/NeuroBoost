#pragma once

#include <cstring>
#include <random>

#include "../common/Constants.h"

// DSP algorithm implementations.
// Realtime-safe: no heap allocations, no exceptions, no I/O.
class Algorithms
{
public:
  // -------------------------------------------------------------------------
  // A0: Euclidean rhythm (Bjorklund algorithm)
  //
  // Distribute 'hits' active beats as evenly as possible among 'total' steps,
  // then rotate by 'rotation' positions.
  //   total      - total number of steps (clamped to [0, outputSize])
  //   hits       - number of active beats (clamped to [0, total])
  //   rotation   - rotation offset (safe-modulo applied)
  //   output     - caller-supplied bool array, length >= outputSize
  //   outputSize - capacity of output[]
  // -------------------------------------------------------------------------
  static void generateEuclidean(int total, int hits, int rotation,
                                bool* output, int outputSize)
  {
    if (outputSize <= 0 || output == nullptr)
      return;

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

    bool pattern[MAX_STEPS] = {};

    int perHit = total / hits;
    int extra  = total % hits;

    int pos = 0;
    for (int h = 0; h < hits && pos < total; ++h)
    {
      pattern[pos] = true;
      pos += perHit;
      if (h < extra) ++pos;
    }

    int rot = ((rotation % total) + total) % total;
    for (int i = 0; i < total; ++i)
      output[i] = pattern[(i + rot) % total];
  }

  // -------------------------------------------------------------------------
  // A1: Fibonacci rhythm generator
  //
  // Generate Fibonacci numbers (1, 1, 2, 3, 5, 8, 13, ...) and mark the
  // positions (fib_number % stepCount) as active.
  //   stepCount  - number of steps to generate (clamped to outputSize)
  //   output     - caller-supplied bool array
  //   outputSize - capacity of output[]
  // -------------------------------------------------------------------------
  static void generateFibonacci(int stepCount, bool* output, int outputSize);

  // -------------------------------------------------------------------------
  // A2: L-System pattern generator
  //
  // Expand an L-System string from axiom using rules for 'A' and 'B', then
  // map the result to a boolean pattern ('A' = note-on, other = rest).
  //   axiom      - starting character (e.g. 'A')
  //   ruleA      - replacement string for character 'A'
  //   ruleB      - replacement string for character 'B'
  //   iterations - number of expansion steps (capped at MAX_LSYSTEM_LENGTH)
  //   output     - caller-supplied bool array
  //   outputSize - capacity of output[]
  //
  // Uses two fixed char[MAX_LSYSTEM_LENGTH] buffers (static); never allocates on heap.
  // IMPORTANT: Must be called from the control thread only, not from processBlock().
  // -------------------------------------------------------------------------
  static void generateLSystem(char axiom, const char* ruleA, const char* ruleB,
                              int iterations, bool* output, int outputSize);

  // -------------------------------------------------------------------------
  // A3: Cellular Automata (Wolfram 1D)
  //
  // Evolve a 1D cellular automaton using an 8-bit Wolfram rule number.
  // The initial state has a single cell set in the centre. After 'iterations'
  // generations the bottom row is mapped to the output pattern.
  //   rule       - Wolfram rule number 0-255
  //   steps      - number of columns (clamped to MAX_CA_WIDTH)
  //   iterations - number of generations (clamped to MAX_CA_HEIGHT - 1)
  //   output     - caller-supplied bool array
  //   outputSize - capacity of output[]
  //
  // Uses a fixed bool[MAX_CA_HEIGHT][MAX_CA_WIDTH] grid (static).
  // IMPORTANT: Must be called from the control thread only, not from processBlock().
  // -------------------------------------------------------------------------
  static void generateCellularAutomata(int rule, int steps, int iterations,
                                       bool* output, int outputSize);

  // -------------------------------------------------------------------------
  // A4: Fractal mapping (Mandelbrot set)
  //
  // Sample a horizontal line through the Mandelbrot set and map iteration
  // counts to a rhythm pattern.  The function is intended to be called when
  // parameters change, NOT inside the audio callback.
  //   steps           - number of sample points (clamped to outputSize)
  //   cx, cy          - centre of the sampling line
  //   zoom            - horizontal span of the line
  //   maxIter         - maximum Mandelbrot iterations (clamped to MAX_FRACTAL_ITERATIONS)
  //   threshold       - iteration count above which a step is active
  //   iterationCounts - output array of raw iteration values (length >= outputSize)
  //   output          - caller-supplied bool array
  //   outputSize      - capacity of output[] and iterationCounts[]
  // -------------------------------------------------------------------------
  static void generateFractal(int steps, double cx, double cy, double zoom,
                              int maxIter, int threshold,
                              int* iterationCounts, bool* output, int outputSize);

  // -------------------------------------------------------------------------
  // A5: Markov chain pitch sequence generator
  //
  // Generate a sequence of pitch classes (0-11) using a first-order Markov
  // chain.  If a row in transitionMatrix sums to 0, uniform distribution is
  // used as fallback.
  //   transitionMatrix - 12x12 probability table (row = current note, col = next)
  //   startNote        - initial pitch class (0-11)
  //   steps            - number of notes to generate
  //   rng              - caller's mt19937 (ensures determinism)
  //   output           - caller-supplied int array of pitch classes (0-11)
  //   outputSize       - capacity of output[]
  // -------------------------------------------------------------------------
  static void generateMarkov(const double transitionMatrix[12][12],
                             int startNote, int steps,
                             std::mt19937& rng,
                             int* output, int outputSize);

  // -------------------------------------------------------------------------
  // A6: Probability / weighted random
  //
  // For each step i, fire (output[i] = true) when a uniform random draw is
  // less than or equal to probabilities[i].
  //   probabilities - per-step probability [0.0, 1.0]; length >= steps
  //   steps         - number of steps (clamped to outputSize)
  //   rng           - caller's mt19937 (ensures determinism)
  //   output        - caller-supplied bool array
  //   outputSize    - capacity of output[]
  // -------------------------------------------------------------------------
  static void generateProbability(const double* probabilities, int steps,
                                  std::mt19937& rng,
                                  bool* output, int outputSize);

  // -------------------------------------------------------------------------
  // Preset Markov transition matrices (constexpr — no heap, no I/O).
  // Rows represent the current pitch class; columns represent the next pitch
  // class.  Each row sums to 1.0.
  //   Index 0 = Blues, 1 = Jazz, 2 = Minimal
  // -------------------------------------------------------------------------

  // Blues scale (C Eb F Gb G Bb) emphasis
  static constexpr double MARKOV_PRESET_BLUES[12][12] = {
    //  C     C#    D     Eb    E     F     Gb    G     Ab    A     Bb    B
    {0.15, 0.00, 0.00, 0.20, 0.00, 0.20, 0.10, 0.20, 0.00, 0.00, 0.15, 0.00}, // C
    {0.30, 0.00, 0.10, 0.15, 0.00, 0.15, 0.10, 0.10, 0.00, 0.00, 0.10, 0.00}, // C#
    {0.10, 0.00, 0.00, 0.30, 0.00, 0.20, 0.10, 0.20, 0.00, 0.00, 0.10, 0.00}, // D
    {0.20, 0.00, 0.00, 0.15, 0.00, 0.30, 0.10, 0.15, 0.00, 0.00, 0.10, 0.00}, // Eb
    {0.10, 0.00, 0.00, 0.20, 0.00, 0.30, 0.10, 0.20, 0.00, 0.00, 0.10, 0.00}, // E
    {0.10, 0.00, 0.00, 0.20, 0.00, 0.10, 0.20, 0.20, 0.00, 0.00, 0.20, 0.00}, // F
    {0.10, 0.00, 0.00, 0.10, 0.00, 0.30, 0.00, 0.30, 0.00, 0.00, 0.20, 0.00}, // Gb
    {0.20, 0.00, 0.00, 0.10, 0.00, 0.20, 0.10, 0.10, 0.00, 0.00, 0.30, 0.00}, // G
    {0.20, 0.00, 0.00, 0.10, 0.00, 0.10, 0.10, 0.20, 0.00, 0.00, 0.30, 0.00}, // Ab
    {0.10, 0.00, 0.00, 0.20, 0.00, 0.10, 0.10, 0.20, 0.00, 0.00, 0.30, 0.00}, // A
    {0.30, 0.00, 0.00, 0.10, 0.00, 0.20, 0.10, 0.20, 0.00, 0.00, 0.10, 0.00}, // Bb
    {0.40, 0.00, 0.00, 0.10, 0.00, 0.20, 0.10, 0.10, 0.00, 0.00, 0.10, 0.00}, // B
  };

  // Jazz: chromatic approach tones, 4th/5th movement
  static constexpr double MARKOV_PRESET_JAZZ[12][12] = {
    //  C     C#    D     Eb    E     F     Gb    G     Ab    A     Bb    B
    {0.10, 0.05, 0.10, 0.05, 0.10, 0.10, 0.05, 0.15, 0.05, 0.10, 0.05, 0.10}, // C
    {0.15, 0.05, 0.10, 0.10, 0.05, 0.10, 0.10, 0.10, 0.05, 0.10, 0.05, 0.05}, // C#
    {0.10, 0.05, 0.10, 0.05, 0.15, 0.10, 0.05, 0.15, 0.05, 0.10, 0.05, 0.05}, // D
    {0.10, 0.10, 0.05, 0.10, 0.05, 0.15, 0.10, 0.10, 0.05, 0.05, 0.10, 0.05}, // Eb
    {0.15, 0.05, 0.05, 0.05, 0.10, 0.10, 0.05, 0.15, 0.05, 0.10, 0.05, 0.10}, // E
    {0.10, 0.05, 0.10, 0.10, 0.05, 0.10, 0.10, 0.10, 0.05, 0.05, 0.10, 0.10}, // F
    {0.10, 0.10, 0.05, 0.10, 0.05, 0.15, 0.05, 0.10, 0.10, 0.05, 0.10, 0.05}, // Gb
    {0.15, 0.05, 0.10, 0.05, 0.10, 0.10, 0.05, 0.10, 0.05, 0.10, 0.05, 0.10}, // G
    {0.10, 0.10, 0.05, 0.10, 0.05, 0.10, 0.10, 0.15, 0.05, 0.05, 0.10, 0.05}, // Ab
    {0.10, 0.05, 0.10, 0.05, 0.10, 0.10, 0.05, 0.15, 0.05, 0.10, 0.05, 0.10}, // A
    {0.15, 0.05, 0.05, 0.10, 0.05, 0.10, 0.10, 0.10, 0.05, 0.05, 0.10, 0.10}, // Bb
    {0.15, 0.05, 0.10, 0.05, 0.10, 0.10, 0.05, 0.10, 0.05, 0.05, 0.10, 0.10}, // B
  };

  // Minimal: high self-transition with half-step neighbours
  static constexpr double MARKOV_PRESET_MINIMAL[12][12] = {
    //  C     C#    D     Eb    E     F     Gb    G     Ab    A     Bb    B
    {0.50, 0.25, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.25}, // C
    {0.25, 0.50, 0.25, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00}, // C#
    {0.00, 0.25, 0.50, 0.25, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00}, // D
    {0.00, 0.00, 0.25, 0.50, 0.25, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00}, // Eb
    {0.00, 0.00, 0.00, 0.25, 0.50, 0.25, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00}, // E
    {0.00, 0.00, 0.00, 0.00, 0.25, 0.50, 0.25, 0.00, 0.00, 0.00, 0.00, 0.00}, // F
    {0.00, 0.00, 0.00, 0.00, 0.00, 0.25, 0.50, 0.25, 0.00, 0.00, 0.00, 0.00}, // Gb
    {0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.25, 0.50, 0.25, 0.00, 0.00, 0.00}, // G
    {0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.25, 0.50, 0.25, 0.00, 0.00}, // Ab
    {0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.25, 0.50, 0.25, 0.00}, // A
    {0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.25, 0.50, 0.25}, // Bb
    {0.25, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.25, 0.50}, // B
  };
};
