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

int parseMidiTrack(FILE* f) {
    int track_length, var_len, event_type;
    Event event_data;
    int found_end_of_track = 0;
    TrackData* track_data = malloc(sizeof(TrackData));
    track_data->event_queue = createEventQueue(64);
    if (checkMagicNumber(f, MTRK_MAGIC)) {
        printf("Not a valid MIDI file\n");
        return 1;
    }
    track_length = (fgetc(f) << 24) + (fgetc(f) << 16) + (fgetc(f) << 8) + fgetc(f);
    while (!found_end_of_track) {
        var_len = parseVariableLength(f);
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
                    return 1;
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
                addEventToQueue(track_data->event_queue, event_data);
            }
            switch (event_type & 0xF0) {
            case MIDI_NOTE_OFF:
                event_data.offset = var_len;
                event_data.event_type = EVENT_NOTE_OFF;
                event_data.value = (fgetc(f) << 8) | fgetc(f);
                addEventToQueue(track_data->event_queue, event_data);
                break;
            case MIDI_NOTE_ON:
                event_data.offset = var_len;
                event_data.event_type = EVENT_NOTE_OFF;
                event_data.value = (fgetc(f) << 8) | fgetc(f);
                addEventToQueue(track_data->event_queue, event_data);
                break;
            case MIDI_AFTER_TOUCH:
            case MIDI_CONTROL_CHANGE:
            case MIDI_PITCH_WHEEL:
                fseek(f, 2, SEEK_CUR);
                break;
            case MIDI_PROGRAM_CHANGE:
            case MIDI_CHANNEL_PRESSURE:
                fseek(f, 1, SEEK_CUR);
                break;
            default:
                printf("Invalid MIDI event\n");
                return 1;
            }
        }
    }
    trimAndLockEventQueue(track_data->event_queue);
    printf("Event count: %d\n\n", track_data->event_queue->size);
    destroyEventQueue(track_data->event_queue);
    return 0;
}

MidiData* parseMidiFile(char* filename) {
    int format, track_count, division;
    FILE* f = fopen(filename, "rb");
    if (!f) {
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
    for (int i = 0; i < track_count; i++) {
        if (parseMidiTrack(f)) {
            return NULL;
        }
    }
    MidiData* midi_data = malloc(sizeof(MidiData));
    midi_data->ticks_per_quater_note = division;
    midi_data->track_count = track_count;
    return midi_data;
}
