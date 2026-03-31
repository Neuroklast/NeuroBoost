#pragma once

enum class GenerationMode
{
  Euclidean,
  Fibonacci,
  LSystem,
  CellularAutomata,
  Markov,
  Fractal,
  Probability
};

enum class ScaleMode
{
  Chromatic,
  Major,
  Minor,
  Dorian,
  Phrygian,
  Lydian,
  Mixolydian,
  HarmonicMinor,
  MelodicMinor,
  PentatonicMajor,
  PentatonicMinor,
  Blues,
  WholeTone,
  Diminished,
  Augmented
};

enum class TransportMode
{
  HostSync,
  Internal,
  MidiTrigger
};

enum class ConditionMode
{
  Always,
  EveryN,
  Fill,
  PreFill
};

struct MidiNote
{
  int pitch;
  int velocity;
  int channel;
  double startBeat;
  double durationBeats;
  int sampleOffset;
};

struct SequenceStep
{
  int pitch;
  double velocity;
  double velocityRange;
  double probability;
  double durationBeats;
  int ratchetCount;
  double ratchetDecay;
  ConditionMode conditionMode;
  int conditionParam;
  double microTiming;
  bool active;
  bool accent;
};

struct ActiveNote
{
  int pitch;
  int channel;
  double noteOffBeat;
  bool active;
};
