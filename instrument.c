#include "instrument.h"
#include "audio_globals.h"
#include "waveform.h"
#include "envelope.h"
#include "math_utils.h"
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

void updateVolumePan(Instrument* inst);

float note_freq(float note_number) {
    return 440.0f * pow(2, (note_number - 69) / 12.0f) / SAMPLE_RATE;
}

Instrument* createInstrument(uint16_t waveform_index, asdr_env env) {
    Instrument* inst = malloc(sizeof(Instrument));
    inst->wf = waveform_index;
    inst->env = env;
    inst->volume = 0.5f;
    inst->pan = 0.5f;
    updateVolumePan(inst);
    for (int i = 0; i < INST_NOTE_LIST_SIZE; i++) {
        inst->notes[i].state = 0;
    }
    inst->enable_flags = 0;
    inst->vibrato = 0.0f;
    inst->vibrato_periods_per_sample = 0.0f;
    inst->vibrato_index = 0.0f;
    return inst;
}

void setInstrumentQueue(Instrument* inst, EventQueue* queue) {
    inst->event_queue = queue;
    inst->ticks_needed = queue->events[queue->index].offset;
}

void advanceByTicks(Instrument* inst, float ticks) {
    EventQueue* event_queue = inst->event_queue;
    inst->ticks_needed -= ticks;
    float tick_offset = ticks + inst->ticks_needed;
    while(inst->ticks_needed <= 0.0f) {
        Event e = event_queue->events[event_queue->index];
        switch(e.event_type) {
        case EVENT_TIME_OFFSET:
            inst->ticks_needed += e.value;
            break;
        case EVENT_NOTE_ON:
            for (int i = 0; i < INST_NOTE_LIST_SIZE; i++) {
                Note* n = &inst->notes[i];
                if (n->state == 0) {
                    n->state = 1;
                    n->note_id = e.value >> 8;
                    n->periods_per_sample = note_freq_lut[n->note_id];
                    n->wf_index = 0.0f;
                    n->current_time = -seconds_per_tick * tick_offset;
                    n->end_time = 10000.0f;
                    n->volume = (e.value & 0xFF) * 0.003922f;
                    FilterState s = {
                        .x1 = 0, .x2 = 0,
                        .y1 = 0, .y2 = 0
                    };
                    n->filter_state = s;
                    if (inst->vibrato > 0.0001f) {
                        n->vibrato = note_freq(n->note_id + inst->vibrato) - n->periods_per_sample;
                    } else {
                        n->vibrato = 0.0f;
                    }
                    break;
                }
            }
            break;
        case EVENT_NOTE_OFF:
            for (int i = 0; i < INST_NOTE_LIST_SIZE; i++) {
                Note* n = &inst->notes[i];
                if (n->state == 1 && n->note_id == e.value >> 8) {
                    n->end_time = n->current_time + seconds_per_tick * tick_offset;
                }
            }
            break;
        }
        event_queue->index++;
        if (event_queue->index == event_queue->tail) {
            event_queue->index = 0;
        }
        inst->ticks_needed += event_queue->events[event_queue->index].offset;
        tick_offset -= inst->ticks_needed;
    }
}

float playInstrument(Instrument* inst) {
    float val = 0.0f;
    float vib = sampleWaveform16(0, inst->vibrato_index);
    for (int i = 0; i < INST_NOTE_LIST_SIZE; i++) {
        Note* n = &inst->notes[i];
        if (n->state && n->current_time > 0.0f) {
            float env_sample = sampleASDREnvelope(&inst->env, n->current_time, n->end_time);
            float v = sampleWaveform16(inst->wf, n->wf_index) * n->volume;
            if (inst->enable_flags & INST_WAVEFORM_MODULATION_FLAG) {
                float v2 = sampleWaveform16(inst->wf2, n->wf_index) * n->volume;
                v = LERP(v, v2, env_sample);
            }
            if (inst->enable_flags & INST_USE_FILTER_FLAG) {
                v = sample_filter(&inst->filter, &n->filter_state, v);
            }
            v *= env_sample;
            val += v;
            n->wf_index += n->periods_per_sample + vib * n->vibrato;
            if (n->wf_index >= 1.0f) {
                n->wf_index -= 1.0f;
            }
        }
        n->current_time += seconds_per_frame;
    }
    inst->vibrato_index += inst->vibrato_periods_per_sample;
    if (inst->vibrato_index >= 1.0f) {
        inst->vibrato_index -= 1.0f;
    }
    return val;
}

void updateInstrumentNoteState(Instrument* inst) {
    for (int i = 0; i < INST_NOTE_LIST_SIZE; i++) {
        Note* n = &inst->notes[i];
        if (n->state && n->current_time - n->end_time > inst->env.release) {
            n->state = 0;
        }
    }
}

void setVolume(Instrument* inst, float volume) {
    inst->volume = volume;
    updateVolumePan(inst);
}

void setPan(Instrument* inst, float pan) {
    inst->pan = pan;
    updateVolumePan(inst);
}

void setVibrato(Instrument* inst, float strength, float freq) {
    inst->vibrato = strength;
    inst->vibrato_periods_per_sample = freq / SAMPLE_RATE;
}

void updateVolumePan(Instrument* inst) {
    inst->pan_l = (1.0f - inst->pan) * inst->volume;
    inst->pan_r = inst->pan * inst->volume;
}
