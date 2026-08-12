#pragma once

#include <stddef.h>

typedef struct Audio Audio;

int audio_init(
    Audio **audio,
    int sample_rate,
    int channels
);

int audio_write(
    Audio *audio,
    const void *data,
    size_t bytes
);

void audio_drain(Audio *audio);

void audio_destroy(Audio *audio);
