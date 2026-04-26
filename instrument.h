#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "waveform.h"
#include "envelope.h"

typedef struct {
    waveform_16* wf;
    asdr_env* env;
    float pan_l;
    float pan_r;
} instrument;

void destroyInstrument(instrument*);

#endif