#pragma once

// Maximum number of steps in a sequence
constexpr int MAX_STEPS = 64;

// Maximum simultaneous notes (for Note-Off tracking)
constexpr int MAX_POLYPHONY = 32;

// Maximum L-System string length (prevent exponential blowup)
constexpr int MAX_LSYSTEM_LENGTH = 1024;

// Maximum Markov chain order
constexpr int MAX_MARKOV_ORDER = 4;

// Maximum Cellular Automata grid dimensions
constexpr int MAX_CA_WIDTH = 64;
constexpr int MAX_CA_HEIGHT = 64;

// Fractal computation limits
constexpr int MAX_FRACTAL_ITERATIONS = 200;

// Minimum tempo (prevent division by zero)
constexpr double MIN_TEMPO = 20.0;
constexpr double MAX_TEMPO = 300.0;

// Default values
constexpr int DEFAULT_STEP_COUNT = 16;
constexpr double DEFAULT_TEMPO = 120.0;
constexpr int DEFAULT_ROOT_NOTE = 60; // C4
constexpr double DEFAULT_PROBABILITY = 1.0;
constexpr double DEFAULT_VELOCITY = 100.0;
constexpr double DEFAULT_DURATION_BEATS = 0.25;
