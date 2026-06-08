CC = gcc
CFLAGS = -Wall

SRC = miniaudio.c audio_globals.c main.c envelope.c waveform.c instrument.c midi_reader.c events.c
OBJ = $(SRC:.c=.o)

default: $(OBJ)
	$(CC) -o audio_test $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o