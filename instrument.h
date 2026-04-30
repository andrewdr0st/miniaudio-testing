#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "waveform.h"
#include "envelope.h"

typedef struct {
    float periods_per_sample;
    float wf_index;
    float current_time;
    float end_time;
} note;

typedef struct {
    waveform_16* wf;
    asdr_env* env;
    float pan_l;
    float pan_r;
    note notes[4];
} instrument;

void destroyInstrument(instrument*);

#endif