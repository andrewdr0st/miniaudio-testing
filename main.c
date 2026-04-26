#include "miniaudio.h"
#include "envelope.h"
#include "waveform.h"
#include "instrument.h"
#include "math_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define SAMPLE_RATE 48000

instrument* inst;
float seconds_per_frame;
float periods_per_sample;
float freq;
float s;
float timestamp;
float master_volume = 0.5f;

void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    float* outBuffer = (float*) pOutput;
    ma_uint32 i_max = frameCount * 2;
    for (int i = 0; i < i_max; i += 2) {
        float val = sampleWaveform16(inst->wf, s);
        val *= sampleASDREnvelope(inst->env, timestamp, 1.0f);
        val *= master_volume;
        outBuffer[i] = val * inst->pan_l;
        outBuffer[i + 1] = val * inst->pan_r;
        s += periods_per_sample;
        if (s >= 1.0f) {
            s -= 1.0f;
        }
        timestamp += seconds_per_frame;
    }
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

    inst = malloc(sizeof(instrument));
    inst->env = createASDREnvelope(0.05f, 0.1f, 0.2f, 0.1f);
    inst->wf = createSquareWave();
    inst->pan_l = 0.5f;
    inst->pan_r = 0.5f;

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return -1;
    }

    ma_device_start(&device);
    getchar();

    destroyInstrument(inst);
    ma_device_uninit(&device);
    return 0;
}