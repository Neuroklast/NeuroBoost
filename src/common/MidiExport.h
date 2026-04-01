#pragma once

#include "Types.h"
#include "Constants.h"
#include <cstdint>

/// Minimal Standard MIDI File (SMF Format 0) writer.
/// Runs on the UI thread only — NOT realtime-safe (uses std::ofstream).
class MidiExport
{
public:
  struct ExportParams
  {
    const SequenceStep* steps;
    int                 stepCount;          ///< Number of steps to export (1..MAX_STEPS)
    double              bpm;                ///< Tempo in beats per minute
    int                 ticksPerQuarterNote; ///< SMF resolution; default 480
    int                 rootNote;           ///< Base MIDI note (0..127)
    ScaleMode           scaleMode;          ///< For display purposes (not used in export)
  };

  /// Write a SMF Format-0 MIDI file to @p filePath.
  /// Returns true on success, false on file I/O error.
  static bool writeToFile(const char* filePath, const ExportParams& params);

private:
  // Variable-length quantity helpers
  static void writeVlq(uint32_t value, uint8_t* out, int& outLen);
  static void appendVlq(uint32_t value, uint8_t* buf, int& pos);
};
