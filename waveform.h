#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <stdint.h>

typedef struct {
    int8_t samples[16];
} waveform_16;

float sampleWaveform16(uint16_t waveform_index, float time);
uint16_t createSineWave();
uint16_t createSquareWave();
uint16_t createQuarterPulse();
uint16_t createSawWave();
uint16_t createTriangleWave();

#endif