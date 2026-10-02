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

#define REVERB_CHANNEL_COUNT 4

typedef struct {
    float feedback;
    float dry;
    float wet;
    uint16_t diffuse_buffer_offsets[REVERB_CHANNEL_COUNT];
    float* diffuse_buffer;
    uint16_t buffer_offsets[REVERB_CHANNEL_COUNT];
    float* buffer;
} Reverb;

Reverb* create_reverb();
void process_reverb(Reverb*, float* samples, int sample_count);

#endif