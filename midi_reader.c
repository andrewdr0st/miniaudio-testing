#include "midi_reader.h"
#include "events.h"
#include <stdio.h>
#include <stdlib.h>

#define MIDI_NOTE_OFF 0x80
#define MIDI_NOTE_ON 0x90
#define MIDI_AFTER_TOUCH 0xA0
#define MIDI_CONTROL_CHANGE 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_CHANNEL_PRESSURE 0xD0
#define MIDI_PITCH_WHEEL 0xE0

#define META_EVENT_SEQUENCE_NUMBER 0x00
#define META_EVENT_TEXT_EVENT 0x01
#define META_EVENT_COPYRIGHT_NOTICE 0x02
#define META_EVENT_TRACK_NAME 0x03
#define META_EVENT_INSTRUMENT_NAME 0x04
#define META_EVENT_LYRIC_TEXT 0x05
#define META_EVENT_MARKER_TEXT 0x06
#define META_EVENT_CUE_POINT 0x07
#define META_EVENT_MIDI_CHANNEL_PREFIX_ASSIGNMENT 0x20
#define META_EVENT_END_OF_TRACK 0x2F
#define META_EVENT_TEMPO_SETTING 0x51
#define META_EVENT_SMPTE_OFFSET 0x54
#define META_EVENT_TIME_SIGNATURE 0x58
#define META_EVENT_KEY_SIGNATURE 0x59
#define META_EVENT_SEQUENCER_SPECIFIC 0x7F

#define TRACK_NAME_MAX_LEN 32

char MTHD_MAGIC[4] = {'M', 'T', 'h', 'd'};
char MTRK_MAGIC[4] = {'M', 'T', 'r', 'k'};

char track_name_buffer[TRACK_NAME_MAX_LEN];

int longest_track_ticks = 0;

int checkMagicNumber(FILE* f, char expected[4]) {
    for (int i = 0; i < 4; i++) {
        if (fgetc(f) != expected[i]) {
            return 1;
        }
    }
    return 0;
}

int parseVariableLength(FILE* f) {
    int val = 0;
    while(1) {
        int v = fgetc(f);
        val |= (v & 0x7F);
        if (v & 0x80) {
            val = val << 7;
        } else {
            break;
        }
    }
    return val;
}

TrackData parseMidiTrack(FILE* f) {
    int var_len, event_type;
    Event event_data;
    EventQueue* event_queue;
    TrackData track_data;
    track_data.event_queue = NULL;
    int found_end_of_track = 0;
    if (checkMagicNumber(f, MTRK_MAGIC)) {
        printf("Not a valid MIDI file\n");
        return track_data;
    }
    int track_length_ticks = 0;
    event_queue = createEventQueue(64);
    fseek(f, 4, SEEK_CUR);
    while (!found_end_of_track) {
        var_len = parseVariableLength(f);
        track_length_ticks += var_len;
        event_type = fgetc(f);
        if (event_type == 0xFF) {
            event_type = fgetc(f);
            var_len = parseVariableLength(f);
            switch (event_type) {
            case META_EVENT_TRACK_NAME:
                if (var_len < TRACK_NAME_MAX_LEN) {
                    for (int i = 0; i < var_len; i++) {
                        track_name_buffer[i] = fgetc(f);
                    }
                    track_name_buffer[var_len] = '\0';
                    printf("Track Name: %s\n", track_name_buffer);
                } else {
                    fseek(f, var_len, SEEK_CUR);
                }
                break;
            case META_EVENT_END_OF_TRACK:
                found_end_of_track = 1;
                break;
            case META_EVENT_TEMPO_SETTING:
                if (var_len != 3) {
                    printf("Invalid tempo setting\n");
                    destroyEventQueue(event_queue);
                    return track_data;
                }
                int tempo = (fgetc(f) << 16) | (fgetc(f) << 8) | fgetc(f);
                printf("Tempo: %d\n", tempo);
                break;
            default:
                fseek(f, var_len, SEEK_CUR);
                break;
            }
        } else if (event_type == 0xF0 || event_type == 0xF7) {
            while (fgetc(f) != 0xF7);
        } else {
            if (var_len > 255) {
                event_data.event_type = EVENT_TIME_OFFSET;
                event_data.offset = 0;
                event_data.value = var_len;
                var_len = 0;
                addEventToQueue(event_queue, event_data);
            }
            switch (event_type & 0xF0) {
            case MIDI_NOTE_OFF:
                event_data.offset = var_len;
                event_data.event_type = EVENT_NOTE_OFF;
                event_data.value = (fgetc(f) << 8) | fgetc(f);
                addEventToQueue(event_queue, event_data);
                break;
            case MIDI_NOTE_ON:
                event_data.offset = var_len;
                int note = fgetc(f);
                int velocity = fgetc(f);
                event_data.event_type = velocity > 0 ? EVENT_NOTE_ON : EVENT_NOTE_OFF;
                event_data.value = (note << 8) | velocity;
                addEventToQueue(event_queue, event_data);
                break;
            case MIDI_AFTER_TOUCH:
            case MIDI_CONTROL_CHANGE:
            case MIDI_PITCH_WHEEL:
                if (var_len > 0) {
                    event_data.event_type = EVENT_TIME_OFFSET;
                    event_data.offset = 0;
                    event_data.value = var_len;
                    var_len = 0;
                    addEventToQueue(event_queue, event_data);
                }
                fseek(f, 2, SEEK_CUR);
                break;
            case MIDI_PROGRAM_CHANGE:
            case MIDI_CHANNEL_PRESSURE:
                if (var_len > 0) {
                    event_data.event_type = EVENT_TIME_OFFSET;
                    event_data.offset = 0;
                    event_data.value = var_len;
                    var_len = 0;
                    addEventToQueue(event_queue, event_data);
                }
                fseek(f, 1, SEEK_CUR);
                break;
            default:
                printf("Invalid MIDI event\n");
                destroyEventQueue(event_queue);
                return track_data;
            }
        }
    }
    event_data.event_type = EVENT_TIME_OFFSET;
    event_data.offset = 0;
    event_data.value = 0;
    addEventToQueue(event_queue, event_data);
    event_queue->tick_length = track_length_ticks;
    if (track_length_ticks > longest_track_ticks) {
        longest_track_ticks = track_length_ticks;
    }
    trimAndLockEventQueue(event_queue);
    printf("Event count: %d\n\n", event_queue->size);
    track_data.event_queue = event_queue;
    return track_data;
}

MidiData* parseMidiFile(char* filename) {
    int format, track_count, division;
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("Unable to open file\n");
        return NULL;
    }
    if (checkMagicNumber(f, MTHD_MAGIC)) {
        printf("Not a valid MIDI file\n");
        return NULL;
    }
    fseek(f, 5, SEEK_CUR);
    format = fgetc(f);
    track_count = (fgetc(f) << 8) | fgetc(f);
    division = (fgetc(f) << 8) | fgetc(f);
    printf("Format: %d, Track Count: %d, Ticks per Qrt Note: %d\n", format, track_count, division);
    MidiData* midi_data = malloc(sizeof(MidiData));
    midi_data->ticks_per_quater_note = division;
    midi_data->track_count = track_count;
    midi_data->tracks = malloc(sizeof(TrackData) * track_count);
    for (int i = 0; i < track_count; i++) {
        TrackData track_data = parseMidiTrack(f);
        if (!track_data.event_queue) {
            for (int j = 0; j < i; j++) {
                destroyEventQueue(midi_data->tracks[j].event_queue);
            }
            free(midi_data->tracks);
            free(midi_data);
            return NULL;
        }
        midi_data->tracks[i] = track_data;
    }
    return midi_data;
}
