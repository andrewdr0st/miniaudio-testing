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


#define STEREO_SPREAD 23

const uint16_t reverb_filter_sizes[COMB_FILTER_COUNT + AP_FILTER_COUNT + 1] = {
    1215, 1293, 1390, 1476, 1548, 1623, 1695, 1760,
    245, 605, 480, 371
};
uint16_t reverb_filter_bounds[(COMB_FILTER_COUNT + AP_FILTER_COUNT) * 2 + 1];
uint16_t reverb_buffer_size = 0;

Reverb* create_reverb() {
    if (reverb_buffer_size == 0) {
        reverb_filter_bounds[0] = 0;
        for (int i = 0; i < COMB_FILTER_COUNT + AP_FILTER_COUNT; i++) {
            reverb_buffer_size += reverb_filter_sizes[i];
            reverb_filter_bounds[i * 2 + 1] = reverb_buffer_size;
            reverb_buffer_size += reverb_filter_sizes[i] + STEREO_SPREAD;
            reverb_filter_bounds[i * 2 + 2] = reverb_buffer_size;
        }
    }
    Reverb* reverb = malloc(sizeof(Reverb));
    reverb->comb_feedback = 0.84f;
    reverb->ap_feedback = 0.5f;
    reverb->damp = 0.2f;
    reverb->dry = 0.3f;
    reverb->wet1 = 0.6f;
    reverb->wet2 = 0.1f;
    for (int i = 0; i < COMB_FILTER_COUNT * 2; i++) {
        reverb->fstore[i] = 0;
    }
    uint16_t offset = 0;
    for (int i = 0; i < COMB_FILTER_COUNT + AP_FILTER_COUNT; i++) {
        reverb->buffer_offsets[i * 2] = offset;
        offset += reverb_filter_sizes[i];
        reverb->buffer_offsets[i * 2 + 1] = offset;
        offset += reverb_filter_sizes[i] + STEREO_SPREAD;
    }
    reverb->buffer = calloc(reverb_buffer_size, sizeof(float));
    return reverb;
}

float process_comb(Reverb* reverb, float in, uint8_t filter_index) {
    uint16_t index = reverb->buffer_offsets[filter_index];
    float output = reverb->buffer[index];
    reverb->fstore[filter_index] = output * (1 - reverb->damp) + reverb->fstore[filter_index] * reverb->damp;
    reverb->buffer[index] = in + reverb->fstore[filter_index] * reverb->comb_feedback;
    reverb->buffer_offsets[filter_index]++;
    if (reverb->buffer_offsets[filter_index] == reverb_filter_bounds[filter_index + 1]) {
        reverb->buffer_offsets[filter_index] = reverb_filter_bounds[filter_index];
    }
    return output;
}

float process_allpass(Reverb* reverb, float in, uint8_t filter_index) {
    uint16_t index = reverb->buffer_offsets[filter_index];
    float buffer_output = reverb->buffer[index];
    float output = -in + buffer_output;
    reverb->buffer[index] = in + buffer_output * reverb->ap_feedback;
    if (reverb->buffer_offsets[filter_index] == reverb_filter_bounds[filter_index + 1]) {
        reverb->buffer_offsets[filter_index] = reverb_filter_bounds[filter_index];
    }
    return output;
}

void process_reverb(Reverb* reverb, float* in, float* out, int sample_count) {
    for (int sample = 0; sample < sample_count; sample++) {
        float in_l = in[sample * 2];
        float in_r = in[sample * 2 + 1];
        float input = (in_l + in_r) * 0.015f;
        float out_l = 0;
        float out_r = 0;
        for (int i = 0; i < COMB_FILTER_COUNT; i++) {
            out_l += process_comb(reverb, input, i * 2);
            out_r += process_comb(reverb, input, i * 2 + 1);
        }
        for (int i = COMB_FILTER_COUNT; i < COMB_FILTER_COUNT + AP_FILTER_COUNT; i++) {
            out_l = process_allpass(reverb, out_l, i * 2);
            out_r = process_allpass(reverb, out_r, i * 2 + 1);
        }
        out[sample * 2] = out_l * reverb->wet1 + out_r * reverb->wet2 + in_l * reverb->dry;
        out[sample * 2 + 1] = out_r * reverb->wet1 + out_l * reverb->wet2 + in_r * reverb->dry;
    }
}


