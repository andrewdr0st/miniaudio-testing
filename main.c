#include "miniaudio.h"
#include <stdio.h>

int count;
float current;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    float* outBuffer = (float*) pOutput;
    for (int i = 0; i < frameCount; i++) {
        int bufferIdx = i * 2;
        outBuffer[bufferIdx] = current;
        outBuffer[bufferIdx + 1] = current;
        count++;
        if (count > 120) {
            count = 0;
            current *= -1.0f;
        }
    }
}

int main() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 48000;
    config.dataCallback = data_callback;
    count = 0;
    current = 0.15f;

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return -1;
    }

    ma_device_start(&device);
    getchar();
    ma_device_uninit(&device);
    return 0;
}