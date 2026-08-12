#pragma once

#include <stddef.h>

typedef struct PulseAudio PulseAudio;

int pulse_init(PulseAudio **pulse,
               int sample_rate,
               int channels);

int pulse_write(PulseAudio *pulse,
                const void *data,
                size_t bytes);

void pulse_drain(PulseAudio *pulse);

void pulse_destroy(PulseAudio *pulse);
