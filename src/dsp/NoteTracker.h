#pragma once

#include "../common/Constants.h"
#include "../common/Types.h"

// Fixed-size active note tracker.
// Realtime-safe: no heap allocations, no exceptions, no mutex.
class NoteTracker
{
public:
  NoteTracker()
  {
    reset();
  }

  // Register a new note-on. Returns false if all slots are taken (MAX_POLYPHONY reached).
  bool noteOn(int pitch, int channel, double noteOffBeat)
  {
    for (int i = 0; i < MAX_POLYPHONY; ++i)
    {
      if (!mNotes[i].active)
      {
        mNotes[i].pitch      = pitch;
        mNotes[i].channel    = channel;
        mNotes[i].noteOffBeat = noteOffBeat;
        mNotes[i].active     = true;
        return true;
      }
    }
    return false; // polyphony overflow
  }

  // Check all active notes against currentBeat.
  // Any note whose noteOffBeat <= currentBeat is written to noteOffsOut[]
  // and its slot is freed. Returns the number of note-offs written.
  // noteOffsOut must have room for at least MAX_POLYPHONY entries.
  int checkNoteOffs(double currentBeat, ActiveNote* noteOffsOut)
  {
    int count = 0;
    for (int i = 0; i < MAX_POLYPHONY; ++i)
    {
      if (mNotes[i].active && currentBeat >= mNotes[i].noteOffBeat)
      {
        if (noteOffsOut)
          noteOffsOut[count] = mNotes[i];
        mNotes[i].active = false;
        ++count;
      }
    }
    return count;
  }

  // Mark all active notes for immediate Note-Off (panic).
  // Writes them to noteOffsOut (must have room for MAX_POLYPHONY entries).
  // Returns the number of notes cleared.
  int panic(ActiveNote* noteOffsOut)
  {
    int count = 0;
    for (int i = 0; i < MAX_POLYPHONY; ++i)
    {
      if (mNotes[i].active)
      {
        if (noteOffsOut)
          noteOffsOut[count] = mNotes[i];
        mNotes[i].active = false;
        ++count;
      }
    }
    return count;
  }

  // Number of currently active notes.
  int activeCount() const
  {
    int count = 0;
    for (int i = 0; i < MAX_POLYPHONY; ++i)
      if (mNotes[i].active) ++count;
    return count;
  }

  // Reset all slots (no notes active).
  void reset()
  {
    for (int i = 0; i < MAX_POLYPHONY; ++i)
    {
      mNotes[i].pitch      = 0;
      mNotes[i].channel    = 0;
      mNotes[i].noteOffBeat = 0.0;
      mNotes[i].active     = false;
    }
  }

  // Read-only access to the internal table (for tests / serialisation).
  const ActiveNote& getNote(int index) const { return mNotes[index]; }

private:
  ActiveNote mNotes[MAX_POLYPHONY];
};
