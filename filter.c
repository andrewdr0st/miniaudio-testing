#include "filter.h"
#include "audio_globals.h"
#include "math_utils.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

Filter create_lowpass(float cutoff, float q) {
    Filter f;
    float w0 = TWO_PI * cutoff / SAMPLE_RATE;
    float alpha = sinf(w0) / (2 * q);
    float c = cosf(w0);
    float a0 = alpha + 1;
    f.b02 = (1 - c) / (2 * a0);
    f.b1 = (1 - c) / a0;
    f.a1 = (-2 * c) / a0;
    f.a2 = (1 - alpha) / a0;
    return f;
}

Filter create_highpass(float cutoff, float q) {
    Filter f;
    float w0 = TWO_PI * cutoff / SAMPLE_RATE;
    float alpha = sinf(w0) / (2 * q);
    float c = cosf(w0);
    float a0 = alpha + 1;
    f.b02 = (1 + c) / (2 * a0);
    f.b1 = -(1 + c) / a0;
    f.a1 = (-2 * c) / a0;
    f.a2 = (1 - alpha) / a0;
    return f;
}

float sample_filter(Filter* f, FilterState* s, float x0) {
    float y0 = f->b02 * x0 + f->b1 * s->x1 + f->b02 * s->x2 - f->a1 * s->y1 - f->a2 * s->y2;
    s->x2 = s->x1;
    s->x1 = x0;
    s->y2 = s->y1;
    s->y1 = y0;
    return y0;
}

#define CHANNEL_0_DELAY 1393
#define CHANNEL_1_DELAY 1967
#define CHANNEL_2_DELAY 2881
#define CHANNEL_3_DELAY 3695

const uint16_t reverb_delays[REVERB_CHANNEL_COUNT] = {CHANNEL_0_DELAY, CHANNEL_1_DELAY, CHANNEL_2_DELAY, CHANNEL_3_DELAY};
const uint16_t reverb_boundaries[REVERB_CHANNEL_COUNT] = {CHANNEL_0_DELAY, CHANNEL_0_DELAY + CHANNEL_1_DELAY, CHANNEL_0_DELAY + CHANNEL_1_DELAY + CHANNEL_2_DELAY, CHANNEL_0_DELAY + CHANNEL_1_DELAY + CHANNEL_2_DELAY + CHANNEL_3_DELAY};
const uint16_t reverb_buffer_size = CHANNEL_0_DELAY + CHANNEL_1_DELAY + CHANNEL_2_DELAY + CHANNEL_3_DELAY;

#define DIFFUSE_0_DELAY 337
#define DIFFUSE_1_DELAY 1489
#define DIFFUSE_2_DELAY 2833
#define DIFFUSE_3_DELAY 4367

const uint16_t diffuse_delays[REVERB_CHANNEL_COUNT] = {DIFFUSE_0_DELAY, DIFFUSE_1_DELAY, DIFFUSE_2_DELAY, DIFFUSE_3_DELAY};
const uint16_t diffuse_boundaries[REVERB_CHANNEL_COUNT] = {DIFFUSE_0_DELAY, DIFFUSE_0_DELAY + DIFFUSE_1_DELAY, DIFFUSE_0_DELAY + DIFFUSE_1_DELAY + DIFFUSE_2_DELAY, DIFFUSE_0_DELAY + DIFFUSE_1_DELAY + DIFFUSE_2_DELAY + DIFFUSE_3_DELAY};
const uint16_t diffuse_buffer_size = DIFFUSE_0_DELAY + DIFFUSE_1_DELAY + DIFFUSE_2_DELAY + DIFFUSE_3_DELAY;

Reverb* create_reverb() {
    Reverb* reverb = malloc(sizeof(Reverb));
    reverb->feedback = 0.8f;
    reverb->dry = 1.0f;
    reverb->wet = 0.5f;
    reverb->buffer_offsets[0] = 0;
    reverb->diffuse_buffer_offsets[0] = 0;
    for (int i = 1; i < REVERB_CHANNEL_COUNT; i++) {
        reverb->buffer_offsets[i] = reverb_boundaries[i - 1];
        reverb->diffuse_buffer_offsets[i] = diffuse_boundaries[i - 1];
    }
    reverb->buffer = calloc(reverb_buffer_size, sizeof(float));
    reverb->diffuse_buffer = calloc(diffuse_buffer_size, sizeof(float));
    return reverb;
}

void process_reverb(Reverb* reverb, float* samples, int sample_count) {
    for (int sample = 0; sample < sample_count; sample++) {
        float in = samples[sample] * 0.25f;
        float d0 = reverb->diffuse_buffer[reverb->diffuse_buffer_offsets[0]];
        float d1 = reverb->diffuse_buffer[reverb->diffuse_buffer_offsets[1]];
        float d2 = reverb->diffuse_buffer[reverb->diffuse_buffer_offsets[2]];
        float d3 = reverb->diffuse_buffer[reverb->diffuse_buffer_offsets[3]];
        reverb->diffuse_buffer[reverb->diffuse_buffer_offsets[0]] = in;
        reverb->diffuse_buffer[reverb->diffuse_buffer_offsets[1]] = in;
        reverb->diffuse_buffer[reverb->diffuse_buffer_offsets[2]] = in;
        reverb->diffuse_buffer[reverb->diffuse_buffer_offsets[3]] = in;
        float md0 = (d0 + d1 + d2 + d3) * 0.5f;
        float md1 = (d0 - d1 + d2 - d3) * 0.5f;
        float md2 = (d0 + d1 - d2 - d3) * 0.5f;
        float md3 = (d0 - d1 - d2 + d3) * 0.5f;
        float r0 = reverb->buffer[reverb->buffer_offsets[0]] * reverb->feedback;
        float r1 = reverb->buffer[reverb->buffer_offsets[1]] * reverb->feedback;
        float r2 = reverb->buffer[reverb->buffer_offsets[2]] * reverb->feedback;
        float r3 = reverb->buffer[reverb->buffer_offsets[3]] * reverb->feedback;
        float m0 = (r0 + r1 + r2 + r3) * 0.5f;
        float m1 = (r0 - r1 + r2 - r3) * 0.5f;
        float m2 = (r0 + r1 - r2 - r3) * 0.5f;
        float m3 = (r0 - r1 - r2 + r3) * 0.5f;
        reverb->buffer[reverb->buffer_offsets[0]] = m0 + md0;
        reverb->buffer[reverb->buffer_offsets[1]] = m1 + md1;
        reverb->buffer[reverb->buffer_offsets[2]] = m2 + md2;
        reverb->buffer[reverb->buffer_offsets[3]] = m3 + md3;
        samples[sample] = samples[sample] * reverb->dry + (m0 + m1 + m2 + m3) * reverb->wet;
        for (int i = 0; i < REVERB_CHANNEL_COUNT; i++) {
            reverb->buffer_offsets[i]++;
            if (reverb->buffer_offsets[i] >= reverb_boundaries[i]) {
                reverb->buffer_offsets[i] -= reverb_delays[i];
            }
            reverb->diffuse_buffer_offsets[i]++;
            if (reverb->diffuse_buffer_offsets[i] >= diffuse_boundaries[i]) {
                reverb->diffuse_buffer_offsets[i] -= diffuse_delays[i];
            }
        }
    }
}
