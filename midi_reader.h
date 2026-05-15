#ifndef MIDI_READER_H
#define MIDI_READER_H

#include <stdint.h>
#include "events.h"

typedef struct {
    EventQueue* event_queue;
} TrackData;

typedef struct {
    uint16_t ticks_per_quater_note;
    uint16_t track_count;
    TrackData* tracks;
} MidiData;

MidiData* parseMidiFile(char* filename);

#endif