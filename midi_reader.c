#include "midi_reader.h"
#include <stdio.h>

#define META_EVENT_TRACK_NAME 0x03
#define META_EVENT_END_OF_TRACK 0x2F

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
    int found_end_of_track = 0;
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
            default:
                fseek(f, var_len, SEEK_CUR);
                break;
            }
        } else {
            if ((event_type >> 4) == 0x0C || (event_type >> 4) == 0x0D) {
                fseek(f, 1, SEEK_CUR);
            } else {
                fseek(f, 2, SEEK_CUR);
            }
        }
    }
    return 0;
}

void parseMidiFile(char* filename) {
    int format, track_count, division;
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return;
    }
    if (checkMagicNumber(f, MTHD_MAGIC)) {
        printf("Not a valid MIDI file\n");
        return;
    }
    fseek(f, 5, SEEK_CUR);
    format = fgetc(f);
    track_count = (fgetc(f) << 8) + fgetc(f);
    division = (fgetc(f) << 8) + fgetc(f);
    printf("Format: %d, Track Count: %d, Ticks per Qrt Note: %d\n", format, track_count, division);
    for (int i = 0; i < track_count; i++) {
        if (parseMidiTrack(f)) {
            return;
        }
    }
}
