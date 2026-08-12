CC = gcc

CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -O2 -D_POSIX_C_SOURCE=200809L

SRC = \
	src/main.c \
	src/screen.c \
	src/input.c \
	src/action.c \
	src/player.c \
	src/render.c \
	src/mpg.c \
	src/metadata.c \
	src/audio/audio.c \
	src/audio/pulse.c \
	src/audio/player.c \
	src/playlist.c \
	src/search.c \
	src/media_keys.c

OUT = maxi-player

LDLIBS = -lmpg123 -lpulse-simple -lpulse -pthread

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDLIBS)

clean:
	rm -f $(OUT)

.PHONY: all clean
