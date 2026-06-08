#include "miniaudio.h"
#include "audio_globals.h"
#include "envelope.h"
#include "waveform.h"
#include "instrument.h"
#include "math_utils.h"
#include "midi_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

Instrument* inst;
float tick_counter = 0.0f;
float master_volume = 0.5f;
int microseconds_per_quarter_note;

void populate_lut() {
    for (int i = 0; i < 128; i++) {
        note_freq_lut[i] = 440.0f * pow(2, (i - 69) / 12.0f) / SAMPLE_RATE;
    }
}

void dataCallback(ma_device* device, void* output_buffer, const void* input_buffer, ma_uint32 frame_count) {
    float* outBuffer = (float*) output_buffer;
    advanceByTicks(inst, ticks_per_frame * frame_count);

    ma_uint32 i_max = frame_count * 2;
    for (int i = 0; i < i_max; i += 2) {
        float val = 0.0f;
        for (int j = 0; j < 4; j++) {
            Note* n = &inst->notes[j];
            if (n->state && n->current_time > 0.0f) {
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

    for (int i = 0; i < 4; i++) {
        Note* n = &inst->notes[i];
        if (n->state && n->current_time - n->end_time > inst->env->release) {
            n->state = 0;
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Enter a filename\n");
        return 1;
    }

    MidiData* midi_data = parseMidiFile(argv[1]);
    if (!midi_data) {
        return 1;
    }

    microseconds_per_quarter_note = 500000;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = SAMPLE_RATE;
    config.dataCallback = dataCallback;
    
    seconds_per_frame = 1.0f / SAMPLE_RATE;

    populate_lut();

    float ticks_per_second = midi_data->ticks_per_quater_note / (microseconds_per_quarter_note * 0.000001);
    ticks_per_frame = ticks_per_second * seconds_per_frame;
    seconds_per_tick = 1.0f / ticks_per_second;
    printf("Ticks per frame: %f\nSeconds per tick: %f\n", ticks_per_frame, seconds_per_tick);

    inst = createInstrument(createSquareWave(), createASDREnvelope(0.05f, 0.1f, 0.2f, 0.1f));
    setInstrumentQueue(inst, midi_data->tracks[3].event_queue);

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