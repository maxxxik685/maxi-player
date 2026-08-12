#pragma once

#include <stddef.h>
#include <sys/types.h>

#include <mpg123.h>

#include "player.h"

typedef struct
{
    mpg123_handle *handle;
} MP3Player;


int mpg_init(void);

int mpg_open(
    MP3Player *mpg,
    PlayerState *player
);

int mpg_read(
    MP3Player *mpg,
    void *buffer,
    size_t buffer_size,
    size_t *done
);

void mpg_rewind(
    MP3Player *mpg
);

long mpg_tell(
    MP3Player *mpg
);

int mpg_get_rate(
    MP3Player *mpg
);

int mpg_get_channels(
    MP3Player *mpg
);

void mpg_close(
    MP3Player *mpg
);

long mpg_position_seconds(
    MP3Player *mpg
);

int mpg_seek_seconds(
    MP3Player *mpg,
    long seconds
);
