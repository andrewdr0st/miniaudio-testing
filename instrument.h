#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "waveform.h"
#include "envelope.h"
#include "events.h"
#include <stdint.h>

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
    float pan_l;
    float pan_r;
    Note notes[4];
    EventQueue* event_queue;
    float ticks_needed;
} Instrument;

Instrument* createInstrument(waveform_16*, asdr_env*);
void destroyInstrument(Instrument*);
void setInstrumentQueue(Instrument*, EventQueue*);
void advanceByTicks(Instrument*, float ticks);

#endif