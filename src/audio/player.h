#pragma once

#include "../mpg.h"
#include "../player.h"
#include "audio.h"

#include <pthread.h>

typedef struct
{
    pthread_t thread;

    pthread_mutex_t mutex;

    MP3Player *mpg;
    Audio *audio;
    PlayerState *player;

    int sample_rate;
    int running;
    int initialized;

} AudioPlayer;

int audio_player_start(
    AudioPlayer *player,
    MP3Player *mpg,
    Audio *audio,
    PlayerState *state
);

void audio_player_stop(
    AudioPlayer *player
);

int audio_player_seek(
    AudioPlayer *player,
    long seconds
);
