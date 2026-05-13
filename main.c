#include "miniaudio.h"
#include "envelope.h"
#include "waveform.h"
#include "instrument.h"
#include "math_utils.h"
#include "midi_reader.h"
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
        float val = 0.0f;
        for (int j = 0; j < 2; j++) {
            note* n = &inst->notes[j];
            if (n->current_time > 0.0f) {
                float v = sampleWaveform16(inst->wf, n->wf_index);
                v *= sampleASDREnvelope(inst->env, n->current_time, n->end_time);
                val += v;
                n->wf_index += n->periods_per_sample;
                if (n->wf_index >= 1.0f) {
                    n->wf_index -= 1.0f;
                }
            }
            n->current_time += seconds_per_frame;
        }
        val *= master_volume;
        outBuffer[i] = val * inst->pan_l;
        outBuffer[i + 1] = val * inst->pan_r;
    }
}

int main() {
    parseMidiFile("midi/megalo.mid");

    return 0;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = SAMPLE_RATE;
    config.dataCallback = dataCallback;
    
    seconds_per_frame = 1.0f / SAMPLE_RATE;

    inst = malloc(sizeof(instrument));
    inst->env = createASDREnvelope(0.05f, 0.1f, 0.2f, 0.1f);
    inst->wf = createSquareWave();
    inst->pan_l = 0.5f;
    inst->pan_r = 0.5f;

    inst->notes[0].periods_per_sample = 440.0f / SAMPLE_RATE;
    inst->notes[0].wf_index = 0.0f;
    inst->notes[0].current_time = -0.5f;
    inst->notes[0].end_time = 1.6f;

    inst->notes[1].periods_per_sample = 498.0f / SAMPLE_RATE;
    inst->notes[1].wf_index = 0.0f;
    inst->notes[1].current_time = -1.0f;
    inst->notes[1].end_time = 0.8f;

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