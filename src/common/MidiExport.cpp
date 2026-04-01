// MidiExport.cpp — Standard MIDI File (SMF Format 0) writer.
// Runs on the UI thread — may use file I/O, heap, etc.

#include "MidiExport.h"

#include <cstring>
#include <fstream>
#include <vector>

// ---------------------------------------------------------------------------
// VLQ helpers
// ---------------------------------------------------------------------------

/// Encode @p value as a variable-length quantity into @p buf starting at @p pos.
void MidiExport::appendVlq(uint32_t value, uint8_t* buf, int& pos)
{
  // Encode up to 4 bytes, MSB first with continuation bits
  uint8_t bytes[4];
  int len = 0;
  bytes[len++] = static_cast<uint8_t>(value & 0x7F);
  value >>= 7;
  while (value)
  {
    bytes[len++] = static_cast<uint8_t>((value & 0x7F) | 0x80);
    value >>= 7;
  }
  // Write most-significant byte first
  for (int i = len - 1; i >= 0; --i)
    buf[pos++] = bytes[i];
}

/// Write @p value as VLQ into @p out buffer; set @p outLen to number of bytes written.
void MidiExport::writeVlq(uint32_t value, uint8_t* out, int& outLen)
{
  outLen = 0;
  appendVlq(value, out, outLen);
}

// ---------------------------------------------------------------------------
// Big-endian write helpers (MIDI uses big-endian)
// ---------------------------------------------------------------------------

static void writeBE16(std::vector<uint8_t>& v, uint16_t x)
{
  v.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
  v.push_back(static_cast<uint8_t>( x       & 0xFF));
}

static void writeBE32(std::vector<uint8_t>& v, uint32_t x)
{
  v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
  v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
  v.push_back(static_cast<uint8_t>((x >>  8) & 0xFF));
  v.push_back(static_cast<uint8_t>( x        & 0xFF));
}

static void patchBE32(std::vector<uint8_t>& v, size_t offset, uint32_t x)
{
  v[offset + 0] = static_cast<uint8_t>((x >> 24) & 0xFF);
  v[offset + 1] = static_cast<uint8_t>((x >> 16) & 0xFF);
  v[offset + 2] = static_cast<uint8_t>((x >>  8) & 0xFF);
  v[offset + 3] = static_cast<uint8_t>( x        & 0xFF);
}

// ---------------------------------------------------------------------------
// Main export function
// ---------------------------------------------------------------------------

bool MidiExport::writeToFile(const char* filePath, const ExportParams& params)
{
  if (!filePath || !params.steps || params.stepCount < 1)
    return false;

  const int tpq   = (params.ticksPerQuarterNote > 0) ? params.ticksPerQuarterNote : 480;
  const double bpm = (params.bpm > 0.0) ? params.bpm : 120.0;

  // Each step = 1 quarter note in ticks
  // One step = (4 / stepCount) quarter-notes, but for simplicity we treat
  // each step as having a duration equal to one beat (= tpq ticks).
  // The actual note duration per step is step.durationBeats * tpq ticks.
  const int stepTicks = tpq; // one step = one beat

  // Tempo in microseconds per quarter note
  const uint32_t usPerBeat = static_cast<uint32_t>(60'000'000.0 / bpm);

  // Build track data
  std::vector<uint8_t> track;
  track.reserve(1024);

  // --- Tempo meta event (delta=0) ---
  // Delta time: VLQ(0)
  track.push_back(0x00);
  // Meta event: FF 51 03 tt tt tt
  track.push_back(0xFF);
  track.push_back(0x51);
  track.push_back(0x03);
  track.push_back(static_cast<uint8_t>((usPerBeat >> 16) & 0xFF));
  track.push_back(static_cast<uint8_t>((usPerBeat >>  8) & 0xFF));
  track.push_back(static_cast<uint8_t>( usPerBeat        & 0xFF));

  // --- Time signature meta event (delta=0): 4/4 ---
  track.push_back(0x00);
  track.push_back(0xFF);
  track.push_back(0x58);
  track.push_back(0x04);
  track.push_back(0x04); // numerator
  track.push_back(0x02); // denominator (2 = quarter note)
  track.push_back(0x18); // MIDI clocks per click
  track.push_back(0x08); // 32nd notes per quarter

  // --- Note events ---
  // We collect (absoluteTick, eventByte, byte1, byte2) then sort by tick
  struct Event {
    uint32_t tick;
    uint8_t  status;
    uint8_t  data1;
    uint8_t  data2;
  };
  std::vector<Event> events;
  events.reserve(static_cast<size_t>(params.stepCount) * 4);

  for (int i = 0; i < params.stepCount && i < MAX_STEPS; ++i)
  {
    const SequenceStep& step = params.steps[i];
    if (!step.active) continue;

    int pitch = step.pitch;
    if (pitch < 0)   pitch = 0;
    if (pitch > 127) pitch = 127;

    int vel = static_cast<int>(step.velocity);
    if (vel < 1)   vel = 64;
    if (vel > 127) vel = 127;
    if (step.accent) vel = (vel * 120 / 100 > 127) ? 127 : vel * 120 / 100;

    int ratchetCount = step.ratchetCount;
    if (ratchetCount < 1) ratchetCount = 1;
    if (ratchetCount > 8) ratchetCount = 8;

    const uint32_t stepStartTick = static_cast<uint32_t>(i) * static_cast<uint32_t>(stepTicks);
    const uint32_t ratchetTicks  = static_cast<uint32_t>(stepTicks) / static_cast<uint32_t>(ratchetCount);
    const uint32_t noteDurTicks  =
        static_cast<uint32_t>(step.durationBeats * tpq / static_cast<double>(ratchetCount));

    for (int r = 0; r < ratchetCount; ++r)
    {
      const uint32_t onTick  = stepStartTick + static_cast<uint32_t>(r) * ratchetTicks;
      const uint32_t offTick = onTick + (noteDurTicks > 0 ? noteDurTicks : ratchetTicks / 2);

      // Decay velocity for ratchets
      int rVel = static_cast<int>(static_cast<double>(vel) *
                                  std::pow(step.ratchetDecay, static_cast<double>(r)));
      if (rVel < 1)   rVel = 1;
      if (rVel > 127) rVel = 127;

      events.push_back({ onTick,  0x90, static_cast<uint8_t>(pitch), static_cast<uint8_t>(rVel) });
      events.push_back({ offTick, 0x80, static_cast<uint8_t>(pitch), 0x00 });
    }
  }

  // Sort events by tick (stable sort preserves note-off before note-on at same tick)
  std::stable_sort(events.begin(), events.end(),
    [](const Event& a, const Event& b) { return a.tick < b.tick; });

  // Encode events with delta times
  uint32_t lastTick = 0;
  uint8_t vlqBuf[4];
  int vlqLen = 0;
  for (const auto& ev : events)
  {
    uint32_t delta = ev.tick - lastTick;
    lastTick = ev.tick;
    writeVlq(delta, vlqBuf, vlqLen);
    for (int k = 0; k < vlqLen; ++k)
      track.push_back(vlqBuf[k]);
    track.push_back(ev.status);
    track.push_back(ev.data1);
    track.push_back(ev.data2);
  }

  // --- End-of-track meta event ---
  track.push_back(0x00); // delta = 0
  track.push_back(0xFF);
  track.push_back(0x2F);
  track.push_back(0x00);

  // Build complete MIDI file in memory
  std::vector<uint8_t> file;
  file.reserve(14 + 8 + track.size());

  // MThd chunk: format=0, nTracks=1, division=tpq
  file.push_back('M'); file.push_back('T'); file.push_back('h'); file.push_back('d');
  writeBE32(file, 6);           // chunk length
  writeBE16(file, 0);           // format 0
  writeBE16(file, 1);           // 1 track
  writeBE16(file, static_cast<uint16_t>(tpq)); // ticks per quarter note

  // MTrk chunk
  file.push_back('M'); file.push_back('T'); file.push_back('r'); file.push_back('k');
  size_t lenOffset = file.size();
  writeBE32(file, 0); // placeholder — patched below
  size_t trackStart = file.size();
  for (uint8_t b : track)
    file.push_back(b);
  // Patch track length
  patchBE32(file, lenOffset, static_cast<uint32_t>(file.size() - trackStart));

  // Write to file
  std::ofstream out(filePath, std::ios::binary);
  if (!out.is_open())
    return false;
  out.write(reinterpret_cast<const char*>(file.data()),
            static_cast<std::streamsize>(file.size()));
  return out.good();
}
