CC = gcc
CFLAGS = -Wall

SRC = main.c miniaudio.c envelope.c waveform.c
OBJ = $(SRC:.c=.o)

default: $(OBJ)
	$(CC) -o audio_test $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o