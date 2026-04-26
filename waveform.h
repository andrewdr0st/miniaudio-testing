#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <stdint.h>

typedef struct {
    int8_t samples[16];
} waveform_16;

float sampleWaveform16(waveform_16*, float time);
waveform_16* createSineWave();
waveform_16* createSquareWave();
waveform_16* createSawWave();
waveform_16* createTriangleWave();

#endif