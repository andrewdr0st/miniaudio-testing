#include "instrument.h"
#include "audio_globals.h"
#include "waveform.h"
#include "envelope.h"
#include <stdlib.h>
#include <stdio.h>

void updateVolumePan(Instrument* inst);

Instrument* createInstrument(waveform_16* wf, asdr_env* env) {
    Instrument* inst = malloc(sizeof(Instrument));
    inst->wf = wf;
    inst->env = env;
    inst->volume = 0.5f;
    inst->pan = 0.5f;
    updateVolumePan(inst);
    for (int i = 0; i < INST_NOTE_LIST_SIZE; i++) {
        inst->notes[i].state = 0;
    }
    return inst;
}

void destroyInstrument(Instrument* inst) {
    free(inst->wf);
    free(inst->env);
    free(inst);
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
    for (int i = 0; i < INST_NOTE_LIST_SIZE; i++) {
        Note* n = &inst->notes[i];
        if (n->state && n->current_time > 0.0f) {
            float v = sampleWaveform16(inst->wf, n->wf_index);
            v *= sampleASDREnvelope(inst->env, n->current_time, n->end_time);
            val += v;
            n->wf_index += n->periods_per_sample;
            if (n->wf_index >= 1.0f) {
                n->wf_index -= 1.0f;
            }
        }
        n->current_time += seconds_per_frame;
    }
    return val;
}

void updateInstrumentNoteState(Instrument* inst) {
    for (int i = 0; i < INST_NOTE_LIST_SIZE; i++) {
        Note* n = &inst->notes[i];
        if (n->state && n->current_time - n->end_time > inst->env->release) {
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

void updateVolumePan(Instrument* inst) {
    inst->pan_l = (1.0f - inst->pan) * inst->volume;
    inst->pan_r = inst->pan * inst->volume;
}
