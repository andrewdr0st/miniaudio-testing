#include "instrument.h"
#include "audio_globals.h"
#include <stdlib.h>

Instrument* createInstrument(waveform_16* wf, asdr_env* env) {
    Instrument* inst = malloc(sizeof(Instrument));
    inst->wf = wf;
    inst->env = env;
    inst->pan_l = 0.5f;
    inst->pan_r = 0.5f;
    for (int i = 0; i < 4; i++) {
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
            for (int i = 0; i < 4; i++) {
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
            for (int i = 0; i < 4; i++) {
                Note* n = &inst->notes[i];
                if (n->state == 1 && n->note_id == e.value >> 8) {
                    n->end_time = n->current_time + seconds_per_tick * tick_offset;
                    break;
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