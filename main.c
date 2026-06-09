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

Instrument** instruments;
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

    for (int i = 0; i < 2; i++) {
        updateInstrumentNoteState(instruments[i]);
        advanceByTicks(instruments[i], ticks_per_frame * frame_count);
    }
    
    ma_uint32 i_max = frame_count * 2;
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < i_max; i += 2) {
            float val = playInstrument(instruments[j]);
            val *= master_volume;
            outBuffer[i] += val * instruments[j]->pan_l;
            outBuffer[i + 1] += val * instruments[j]->pan_r;
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

    instruments = malloc(sizeof(Instrument*) * 2);

    instruments[0] = createInstrument(createSquareWave(), createASDREnvelope(0.02f, 0.05f, 0.2f, 0.08f));
    setInstrumentQueue(instruments[0], midi_data->tracks[3].event_queue);

    instruments[1] = createInstrument(createSawWave(), createASDREnvelope(0.04f, 0.07f, 0.25f, 0.09f));
    setInstrumentQueue(instruments[1], midi_data->tracks[6].event_queue);

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return -1;
    }

    ma_device_start(&device);
    getchar();

    destroyInstrument(instruments[0]);
    destroyInstrument(instruments[1]);
    ma_device_uninit(&device);
    return 0;
}