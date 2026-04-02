#include "miniaudio.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    int8_t samples[16];
} waveform_16;

int count;
int max_count;
waveform_16* wf;

void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    float* outBuffer = (float*) pOutput;
    for (int i = 0; i < frameCount; i++) {
        int bufferIdx = i * 2;
        float val = (wf->samples[count / 10] * 0.1f) / 127.0f;
        outBuffer[bufferIdx] = val;
        outBuffer[bufferIdx + 1] = val;
        count++;
        if (count > max_count) {
            count = 0;
        }
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
    config.sampleRate = 48000;
    config.dataCallback = dataCallback;
    count = 0;
    max_count = 160;
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