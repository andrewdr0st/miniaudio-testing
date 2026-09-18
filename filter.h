#ifndef FILTER_H
#define FILTER_H

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

#endif