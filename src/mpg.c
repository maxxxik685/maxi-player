#include "mpg.h"

#include <stdio.h>
#include <mpg123.h>

int mpg_init(void)
{
    if (mpg123_init() != MPG123_OK)
        return 0;

    return 1;
}

int mpg_open(
    MP3Player *mpg,
    PlayerState *player
)
{
    int error;

    mpg123_handle *mh =
        mpg123_new(NULL, &error);

    if (!mh)
        return 0;

    /*
     * Не выводить внутренние сообщения mpg123
     * в терминал maxi-player.
     */
    mpg123_param(
        mh,
        MPG123_ADD_FLAGS,
        MPG123_QUIET,
        0
    );

    if (mpg123_open(
            mh,
            player->path) != MPG123_OK)
    {
        mpg123_delete(mh);
        return 0;
    }

    long rate;
    int channels;
    int encoding;

    if (mpg123_getformat(
            mh,
            &rate,
            &channels,
            &encoding) != MPG123_OK)
    {
        mpg123_close(mh);
        mpg123_delete(mh);

        return 0;
    }

    off_t length =
        mpg123_length(mh);

    if (rate > 0 && length >= 0)
    {
        player->duration =
            (int)(length / rate);
    }

    mpg->handle = mh;

    return 1;
}

int mpg_read(
    MP3Player *mpg,
    void *buffer,
    size_t buffer_size,
    size_t *done
)
{
    int result =
        mpg123_read(
            mpg->handle,
            buffer,
            buffer_size,
            done
        );

    if (result == MPG123_OK)
        return 1;

    if (result == MPG123_DONE)
    {
        *done = 0;
        return 0;
    }

    *done = 0;
    return 0;
}

void mpg_rewind(
    MP3Player *mpg
)
{
    mpg123_seek(
        mpg->handle,
        0,
        SEEK_SET
    );
}

long mpg_tell(
    MP3Player *mpg
)
{
    return mpg123_tell(
        mpg->handle
    );
}

int mpg_get_rate(
    MP3Player *mpg
)
{
    long rate;
    int channels;
    int encoding;

    if (mpg123_getformat(
            mpg->handle,
            &rate,
            &channels,
            &encoding) != MPG123_OK)
    {
        return 0;
    }

    return (int)rate;
}

int mpg_get_channels(
    MP3Player *mpg
)
{
    long rate;
    int channels;
    int encoding;

    if (mpg123_getformat(
            mpg->handle,
            &rate,
            &channels,
            &encoding) != MPG123_OK)
    {
        return 0;
    }

    return channels;
}

long mpg_position_seconds(
    MP3Player *mpg
)
{
    long position =
        mpg123_tell(
            mpg->handle
        );

    long rate =
        mpg_get_rate(mpg);

    if (position < 0 || rate <= 0)
        return 0;

    return position / rate;
}

int mpg_seek_seconds(
    MP3Player *mpg,
    long seconds
)
{
    long rate =
        mpg_get_rate(mpg);

    if (rate <= 0)
        return 0;

    if (seconds < 0)
        seconds = 0;

    off_t position =
        (off_t)seconds * rate;

    off_t result =
        mpg123_seek(
            mpg->handle,
            position,
            SEEK_SET
        );

    return result >= 0;
}

void mpg_close(
    MP3Player *mpg
)
{
    if (!mpg->handle)
        return;

    mpg123_close(
        mpg->handle
    );

    mpg123_delete(
        mpg->handle
    );

    mpg->handle = NULL;
}
