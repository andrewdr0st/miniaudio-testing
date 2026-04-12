#include "miniaudio.h"
#include "envelope.h"
#include "math_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define SAMPLE_RATE 48000
#define ONE_OVER_127 0.007874f

typedef struct {
    int8_t samples[16];
} waveform_16;

waveform_16* wf;
asdr_env env;
float seconds_per_frame;
float periods_per_sample;
float freq;
float s;
float timestamp;
float volume = 0.25f;

void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    float* outBuffer = (float*) pOutput;
    ma_uint32 i_max = frameCount * 2;
    for (int i = 0; i < i_max; i += 2) {
        float idx = s * 16;
        int trunc = (int) idx;
        float w0 = wf->samples[trunc] * ONE_OVER_127;
        float w1 = wf->samples[(trunc + 1) % 16] * ONE_OVER_127;
        float dec = idx - trunc;
        float val = LERP(w0, w1, dec);
        val *= sampleASDREnvelope(env, timestamp, 1.6f);
        val *= volume;
        outBuffer[i] = val;
        outBuffer[i + 1] = val;
        s += periods_per_sample;
        if (s >= 1.0f) {
            s -= 1.0f;
        }
        timestamp += seconds_per_frame;
    }
}

waveform_16* createWaveform() {
    waveform_16* w = malloc(sizeof(waveform_16));
    for (int i = 0; i < 16; i++) {
        w->samples[i] = 127 - (16 * i);
    }
    return w;
}

int main() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = SAMPLE_RATE;
    config.dataCallback = dataCallback;
    seconds_per_frame = 1.0f / SAMPLE_RATE;
    timestamp = 0;
    freq = 440.0f;
    periods_per_sample = freq / SAMPLE_RATE;
    s = 0.0f;
    env.attack = 0.05;
    env.decay = 0.25;
    env.sustain = 0.4;
    env.release = 0.4;
    wf = createWaveform();

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return -1;
    }

    ma_device_start(&device);
    getchar();
    ma_device_uninit(&device);
    return 0;
}