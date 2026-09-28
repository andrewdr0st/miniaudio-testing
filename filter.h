#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

typedef struct {
    float b02, b1;
    float a1, a2;
} Filter;

typedef struct {
    float x1, x2;
    float y1, y2;
} FilterState;

Filter create_lowpass(float cutoff, float q);
Filter create_highpass(float cutoff, float q);
float sample_filter(Filter*, FilterState*, float x0);

#define COMB_FILTER_COUNT 8
#define AP_FILTER_COUNT 4

typedef struct {
    float damp;
    float comb_feedback;
    float ap_feedback;
    float dry;
    float wet1;
    float wet2;
    float fstore[COMB_FILTER_COUNT * 2];
    uint16_t buffer_offsets[(COMB_FILTER_COUNT + AP_FILTER_COUNT) * 2];
    float* buffer;
} Reverb;

Reverb* create_reverb();
void process_reverb(Reverb*, float* in, float* out, int sample_count);

#endif