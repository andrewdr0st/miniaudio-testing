#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "waveform.h"
#include "envelope.h"
#include "filter.h"
#include "events.h"
#include <stdint.h>

#define INST_NOTE_LIST_SIZE 8

#define INST_WAVEFORM_MODULATION_FLAG 0x1
#define INST_USE_FILTER_FLAG 0x2
#define INST_USE_REVERB_FLAG 0x4

typedef struct {
    uint8_t state;
    uint8_t note_id;
    float periods_per_sample;
    float wf_index;
    float current_time;
    float end_time;
    float volume;
    float vibrato;
    FilterState filter_state;
} Note;

typedef struct {
    uint16_t wf;
    uint16_t wf2;
    asdr_env env;
    Filter filter;
    uint8_t enable_flags;
    float volume;
    float pan;
    float pan_l, pan_r;
    Note notes[INST_NOTE_LIST_SIZE];
    EventQueue* event_queue;
    float ticks_needed;
    float vibrato;
    float vibrato_periods_per_sample;
    float vibrato_index;
    Reverb* reverb;
    float sample_buffer[480];
} Instrument;

Instrument* createInstrument(uint16_t waveform_index, asdr_env);
void setInstrumentQueue(Instrument*, EventQueue*);
void advanceByTicks(Instrument*, float ticks);
void playInstrument(Instrument*, float* samples, int sample_count);
void updateInstrumentNoteState(Instrument*);
void setVolume(Instrument*, float volume);
void setPan(Instrument*, float pan);
void setVibrato(Instrument*, float strength, float freq);
float note_freq(float note_number);

#endif