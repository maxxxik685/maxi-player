#include "player.h"

#include <pthread.h>
#include <time.h>

static void sleep_ms(long ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static void *audio_thread(void *data)
{
    AudioPlayer *player = (AudioPlayer *)data;
    unsigned char buffer[8192];

    while (player->running)
    {
        if (player->player->paused)
        {
            sleep_ms(10);
            continue;
        }

        size_t done = 0;
        long sample_position = 0;
        int result;

        pthread_mutex_lock(&player->mutex);

        result = mpg_read(
            player->mpg,
            buffer,
            sizeof(buffer),
            &done
        );

        if (result)
            sample_position = mpg_tell(player->mpg);

        pthread_mutex_unlock(&player->mutex);

        if (!result)
        {
            player->player->ended = 1;
            break;
        }

        if (done > 0)
        {
            audio_write(
                player->audio,
                buffer,
                done
            );

            if (player->sample_rate > 0)
            {
                player->player->position =
                    (int)(sample_position / player->sample_rate);
            }
        }
    }

    return NULL;
}

int audio_player_start(
    AudioPlayer *player,
    MP3Player *mpg,
    Audio *audio,
    PlayerState *state
)
{
    player->mpg = mpg;
    player->audio = audio;
    player->player = state;
    player->sample_rate = mpg_get_rate(mpg);
    player->running = 0;
    player->initialized = 0;

    if (pthread_mutex_init(&player->mutex, NULL) != 0)
        return 0;

    player->running = 1;

    if (pthread_create(
            &player->thread,
            NULL,
            audio_thread,
            player) != 0)
    {
        player->running = 0;
        pthread_mutex_destroy(&player->mutex);
        return 0;
    }

    player->initialized = 1;
    return 1;
}

void audio_player_stop(AudioPlayer *player)
{
    if (!player->initialized)
        return;

    player->running = 0;

    pthread_join(
        player->thread,
        NULL
    );

    pthread_mutex_destroy(&player->mutex);
    player->initialized = 0;
}

int audio_player_seek(
    AudioPlayer *player,
    long seconds
)
{
    if (!player->initialized)
        return 0;

    pthread_mutex_lock(&player->mutex);

    long current = mpg_position_seconds(player->mpg);
    long target = current + seconds;

    if (target < 0)
        target = 0;

    if (player->player->duration > 0 &&
        target >= player->player->duration)
    {
        target = player->player->duration - 1;
    }

    int result = mpg_seek_seconds(
        player->mpg,
        target
    );

    if (result)
        player->player->position = (int)target;

    pthread_mutex_unlock(&player->mutex);

    return result;
}
