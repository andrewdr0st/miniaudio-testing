#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "waveform.h"
#include "envelope.h"
#include "events.h"
#include <stdint.h>

#define INST_NOTE_LIST_SIZE 8

typedef struct {
    uint8_t state;
    uint8_t note_id;
    float periods_per_sample;
    float wf_index;
    float current_time;
    float end_time;
} Note;

typedef struct {
    waveform_16* wf;
    asdr_env* env;
    float volume;
    float pan;
    float pan_l;
    float pan_r;
    Note notes[INST_NOTE_LIST_SIZE];
    EventQueue* event_queue;
    float ticks_needed;
} Instrument;

Instrument* createInstrument(waveform_16*, asdr_env*);
void destroyInstrument(Instrument*);
void setInstrumentQueue(Instrument*, EventQueue*);
void advanceByTicks(Instrument*, float ticks);
float playInstrument(Instrument*);
void updateInstrumentNoteState(Instrument*);
void setVolume(Instrument*, float volume);
void setPan(Instrument*, float pan);

#endif