#pragma once

#include "../common/Types.h"

// Scale quantization with constexpr lookup tables.
// Realtime-safe: all static, no heap allocations, no I/O.
class ScaleQuantizer
{
public:
  // Quantize a MIDI note to the nearest pitch in the given scale.
  // rootNote: root pitch class (0-11, where 0 = C)
  // Returns a MIDI note in [0, 127].
  static int quantize(int midiNote, int rootNote, ScaleMode mode)
  {
    if (mode == ScaleMode::Chromatic)
      return clampMidi(midiNote);

    const int* intervals = getIntervals(mode);
    const int scaleSize   = getScaleSize(mode);

    // Normalize to pitch class relative to root (safe modulo)
    int pitchClass = safeMod(midiNote - rootNote, 12);
    int octave     = (midiNote - rootNote - pitchClass) / 12;

    // Find the nearest scale degree that is >= pitchClass
    int nearest = -1;
    int nearestUp = -1;
    for (int i = 0; i < scaleSize; ++i)
    {
      if (intervals[i] == pitchClass)
      {
        nearest = pitchClass;
        break;
      }
      if (intervals[i] > pitchClass && nearestUp < 0)
        nearestUp = intervals[i];
    }

    if (nearest < 0)
    {
      // Not in scale: snap up or wrap to root of next octave
      if (nearestUp >= 0)
        nearest = nearestUp;
      else
      {
        nearest = 0;
        octave += 1;
      }
    }

    int result = rootNote + octave * 12 + nearest;
    return clampMidi(result);
  }

  // Human-readable scale name.
  static const char* getScaleName(ScaleMode mode)
  {
    switch (mode)
    {
      case ScaleMode::Chromatic:        return "Chromatic";
      case ScaleMode::Major:            return "Major";
      case ScaleMode::Minor:            return "Minor";
      case ScaleMode::Dorian:           return "Dorian";
      case ScaleMode::Phrygian:         return "Phrygian";
      case ScaleMode::Lydian:           return "Lydian";
      case ScaleMode::Mixolydian:       return "Mixolydian";
      case ScaleMode::HarmonicMinor:    return "Harmonic Minor";
      case ScaleMode::MelodicMinor:     return "Melodic Minor";
      case ScaleMode::PentatonicMajor:  return "Pentatonic Major";
      case ScaleMode::PentatonicMinor:  return "Pentatonic Minor";
      case ScaleMode::Blues:            return "Blues";
      case ScaleMode::WholeTone:        return "Whole Tone";
      case ScaleMode::Diminished:       return "Diminished";
      case ScaleMode::Augmented:        return "Augmented";
      default:                          return "Unknown";
    }
  }

  // Number of unique pitches (per octave) in the scale.
  static int getScaleSize(ScaleMode mode)
  {
    switch (mode)
    {
      case ScaleMode::Chromatic:        return 12;
      case ScaleMode::Major:            return 7;
      case ScaleMode::Minor:            return 7;
      case ScaleMode::Dorian:           return 7;
      case ScaleMode::Phrygian:         return 7;
      case ScaleMode::Lydian:           return 7;
      case ScaleMode::Mixolydian:       return 7;
      case ScaleMode::HarmonicMinor:    return 7;
      case ScaleMode::MelodicMinor:     return 7;
      case ScaleMode::PentatonicMajor:  return 5;
      case ScaleMode::PentatonicMinor:  return 5;
      case ScaleMode::Blues:            return 6;
      case ScaleMode::WholeTone:        return 6;
      case ScaleMode::Diminished:       return 8;
      case ScaleMode::Augmented:        return 6;
      default:                          return 12;
    }
  }

private:
  // Semitone intervals from root (ascending) for each scale.
  static const int* getIntervals(ScaleMode mode)
  {
    static constexpr int kChromatic[12]       = {0,1,2,3,4,5,6,7,8,9,10,11};
    static constexpr int kMajor[7]            = {0,2,4,5,7,9,11};
    static constexpr int kMinor[7]            = {0,2,3,5,7,8,10};
    static constexpr int kDorian[7]           = {0,2,3,5,7,9,10};
    static constexpr int kPhrygian[7]         = {0,1,3,5,7,8,10};
    static constexpr int kLydian[7]           = {0,2,4,6,7,9,11};
    static constexpr int kMixolydian[7]       = {0,2,4,5,7,9,10};
    static constexpr int kHarmonicMinor[7]    = {0,2,3,5,7,8,11};
    static constexpr int kMelodicMinor[7]     = {0,2,3,5,7,9,11};
    static constexpr int kPentatonicMajor[5]  = {0,2,4,7,9};
    static constexpr int kPentatonicMinor[5]  = {0,3,5,7,10};
    static constexpr int kBlues[6]            = {0,3,5,6,7,10};
    static constexpr int kWholeTone[6]        = {0,2,4,6,8,10};
    static constexpr int kDiminished[8]       = {0,2,3,5,6,8,9,11};
    static constexpr int kAugmented[6]        = {0,3,4,7,8,11};

    switch (mode)
    {
      case ScaleMode::Major:            return kMajor;
      case ScaleMode::Minor:            return kMinor;
      case ScaleMode::Dorian:           return kDorian;
      case ScaleMode::Phrygian:         return kPhrygian;
      case ScaleMode::Lydian:           return kLydian;
      case ScaleMode::Mixolydian:       return kMixolydian;
      case ScaleMode::HarmonicMinor:    return kHarmonicMinor;
      case ScaleMode::MelodicMinor:     return kMelodicMinor;
      case ScaleMode::PentatonicMajor:  return kPentatonicMajor;
      case ScaleMode::PentatonicMinor:  return kPentatonicMinor;
      case ScaleMode::Blues:            return kBlues;
      case ScaleMode::WholeTone:        return kWholeTone;
      case ScaleMode::Diminished:       return kDiminished;
      case ScaleMode::Augmented:        return kAugmented;
      default:                          return kChromatic;
    }
  }

  static int safeMod(int x, int n)
  {
    return ((x % n) + n) % n;
  }

  static int clampMidi(int note)
  {
    if (note < 0)   return 0;
    if (note > 127) return 127;
    return note;
  }
};
