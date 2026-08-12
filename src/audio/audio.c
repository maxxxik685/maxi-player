#include <stdlib.h>
#include "audio.h"
#include "pulse.h"

struct Audio
{
    PulseAudio *pulse;
};

int audio_init(
    Audio **audio,
    int sample_rate,
    int channels
)
{
    *audio = malloc(sizeof(Audio));

    if (!*audio)
        return 0;

    (*audio)->pulse = NULL;

    if (!pulse_init(
            &(*audio)->pulse,
            sample_rate,
            channels))
    {
        free(*audio);
        *audio = NULL;

        return 0;
    }

    return 1;
}

int audio_write(
    Audio *audio,
    const void *data,
    size_t bytes
)
{
    if (!audio || !audio->pulse)
        return 0;

    return pulse_write(
        audio->pulse,
        data,
        bytes
    );
}

void audio_drain(Audio *audio)
{
    if (!audio || !audio->pulse)
        return;

    pulse_drain(audio->pulse);
}

void audio_destroy(Audio *audio)
{
    if (!audio)
        return;

    if (audio->pulse)
    {
        pulse_destroy(audio->pulse);
        audio->pulse = NULL;
    }

    free(audio);
}
