#include "waveform.h"
#include "math_utils.h"
#include <stdlib.h>

float sampleWaveform16(waveform_16* wf, float time) {
    float index = time * 16;
    int trunc = (int) index;
    float w0 = wf->samples[trunc] * ONE_OVER_127;
    float w1 = wf->samples[(trunc + 1) % 16] * ONE_OVER_127;
    float dec = index - trunc;
    return LERP(w0, w1, dec);
}

waveform_16* createSquareWave() {
    waveform_16* wf = malloc(sizeof(waveform_16));
    for (int i = 0; i < 8; i++) {
        wf->samples[i] = 127;
    }
    for (int i = 8; i < 16; i++) {
        wf->samples[i] = -127;
    }
    return wf;
}

waveform_16* createSawWave() {
    waveform_16* wf = malloc(sizeof(waveform_16));
    for (int i = 0; i < 15; i++) {
        wf->samples[i] = 127 - (17 * i);
    }
    wf->samples[15] = -127;
    return wf;
}
