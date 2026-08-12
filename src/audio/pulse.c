#include "pulse.h"

#include <stdlib.h>

#include <pulse/simple.h>
#include <pulse/error.h>

struct PulseAudio
{
    pa_simple *handle;
};

int pulse_init(
    PulseAudio **pulse,
    int sample_rate,
    int channels
)
{
    *pulse = malloc(sizeof(PulseAudio));

    if (!*pulse)
        return 0;

    (*pulse)->handle = NULL;

    pa_sample_spec spec;

    spec.format = PA_SAMPLE_S16LE;
    spec.rate = sample_rate;
    spec.channels = channels;

    int error;

    (*pulse)->handle =
        pa_simple_new(
            NULL,
            "maxi-player",
            PA_STREAM_PLAYBACK,
            NULL,
            "music",
            &spec,
            NULL,
            NULL,
            &error
        );

    if (!(*pulse)->handle)
    {
        free(*pulse);
        *pulse = NULL;

        return 0;
    }

    return 1;
}

int pulse_write(
    PulseAudio *pulse,
    const void *data,
    size_t bytes
)
{
    if (!pulse || !pulse->handle)
        return 0;

    int error;

    if (pa_simple_write(
            pulse->handle,
            data,
            bytes,
            &error) < 0)
    {
        return 0;
    }

    return 1;
}

void pulse_drain(PulseAudio *pulse)
{
    if (!pulse || !pulse->handle)
        return;

    int error;

    pa_simple_drain(
        pulse->handle,
        &error
    );
}

void pulse_destroy(PulseAudio *pulse)
{
    if (!pulse)
        return;

    if (pulse->handle)
    {
        pa_simple_free(
            pulse->handle
        );

        pulse->handle = NULL;
    }

    free(pulse);
}
