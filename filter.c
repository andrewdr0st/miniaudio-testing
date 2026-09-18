#include "filter.h"
#include "audio_globals.h"
#include <math.h>

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
