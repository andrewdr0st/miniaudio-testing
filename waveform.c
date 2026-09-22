#include "waveform.h"
#include "math_utils.h"
#include <math.h>
#include <stdint.h>

#define MAX_WAVEFORMS 16

waveform_16 waveforms[MAX_WAVEFORMS];
uint16_t waveform_count = 0;

float sampleWaveform16(uint16_t waveform_index, float time) {
    waveform_16* wf = &waveforms[waveform_index];
    float index = time * 16;
    int trunc = (int) index;
    float w0 = wf->samples[trunc % 16] * ONE_OVER_127;
    float w1 = wf->samples[(trunc + 1) % 16] * ONE_OVER_127;
    float dec = index - trunc;
    return LERP(w0, w1, dec);
}

uint16_t createSineWave() {
    if (waveform_count == MAX_WAVEFORMS) {
        return 0;
    }
    waveform_16* wf = &waveforms[waveform_count];
    for (int i = 0; i < 16; i++) {
        wf->samples[i] = sinf((((float)i) / 16.0f) * TWO_PI) * 127;
    }
    uint16_t w_index = waveform_count;
    waveform_count++;
    return w_index;
}

uint16_t createSquareWave() {
    if (waveform_count == MAX_WAVEFORMS) {
        return 0;
    }
    waveform_16* wf = &waveforms[waveform_count];
    for (int i = 0; i < 8; i++) {
        wf->samples[i] = 127;
    }
    for (int i = 8; i < 16; i++) {
        wf->samples[i] = -127;
    }
    uint16_t w_index = waveform_count;
    waveform_count++;
    return w_index;
}

uint16_t createQuarterPulse() {
    if (waveform_count == MAX_WAVEFORMS) {
        return 0;
    }
    waveform_16* wf = &waveforms[waveform_count];
    for (int i = 0; i < 12; i++) {
        wf->samples[i] = 127;
    }
    for (int i = 12; i < 16; i++) {
        wf->samples[i] = -127;
    }
    uint16_t w_index = waveform_count;
    waveform_count++;
    return w_index;
}

uint16_t createSawWave() {
    if (waveform_count == MAX_WAVEFORMS) {
        return 0;
    }
    waveform_16* wf = &waveforms[waveform_count];
    for (int i = 0; i < 15; i++) {
        wf->samples[i] = 127 - (17 * i);
    }
    wf->samples[15] = -127;
    uint16_t w_index = waveform_count;
    waveform_count++;
    return w_index;
}

uint16_t createTriangleWave() {
    if (waveform_count == MAX_WAVEFORMS) {
        return 0;
    }
    waveform_16* wf = &waveforms[waveform_count];
    for (int i = 0; i < 8; i++) {
        int x = i * 32;
        wf->samples[i] = 127 - x;
        wf->samples[i + 8] = -127 + x;
    }
    uint16_t w_index = waveform_count;
    waveform_count++;
    return w_index;
}
