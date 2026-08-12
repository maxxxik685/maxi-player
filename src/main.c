#include "media_keys.h"
#include "search.h"
#include "audio/audio.h"
#include "audio/player.h"

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "metadata.h"
#include "mpg.h"
#include "screen.h"
#include "input.h"
#include "action.h"
#include "player.h"
#include "render.h"
#include "playlist.h"

static void sleep_ms(long ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static long long now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (long long)tv.tv_sec * 1000LL +
           tv.tv_usec / 1000LL;
}

static int load_track(
    PlayerState *player,
    MP3Player *mpg,
    AudioPlayer *audio_player,
    Audio **audio,
    const char *path
)
{
    audio_player_stop(audio_player);
    audio_destroy(*audio);
    *audio = NULL;
    mpg_close(mpg);

    snprintf(player->path, sizeof(player->path), "%s", path);

    player->position = 0;
    player->paused = 0;
    player->ended = 0;

    if (!mpg_open(mpg, player))
        return 0;

    int sample_rate = mpg_get_rate(mpg);
    int channels = mpg_get_channels(mpg);

    if (!audio_init(audio, sample_rate, channels))
    {
        mpg_close(mpg);
        return 0;
    }

    if (!audio_player_start(
            audio_player,
            mpg,
            *audio,
            player))
    {
        audio_destroy(*audio);
        *audio = NULL;
        mpg_close(mpg);
        return 0;
    }

    metadata_load(player->path, player);
    return 1;
}

static int start_track(
    PlayerState *player,
    Playlist *playlist,
    MP3Player *mpg,
    AudioPlayer *audio_player,
    Audio **audio,
    int index
)
{
    if (index < 0 || index >= playlist->count)
        return 0;

    playlist->current = index;

    return load_track(
        player,
        mpg,
        audio_player,
        audio,
        playlist->paths[index]
    );
}

static void search_select_move(SearchState *search, int direction)
{
    if (search->result_count <= 0)
        return;

    if (direction < 0)
    {
        if (search->selected > 0)
            search->selected--;
    }
    else
    {
        if (search->selected < search->result_count - 1)
            search->selected++;
    }
}

static void search_add_key(SearchState *search, Key key)
{
    if (key.type != KEY_CHAR)
        return;

    for (int i = 0; i < key.length; i++)
        search_add_byte(search, key.bytes[i]);
}

int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));

    screen_init();

    if (!input_init())
    {
        screen_shutdown();
        return 1;
    }

    if (!media_keys_init())
    {
        input_shutdown();
        screen_shutdown();
        return 1;
    }

    if (argc < 2)
    {
        media_keys_shutdown();
        input_shutdown();
        screen_shutdown();
        printf("Usage: maxi-player <file1.mp3> [file2.mp3] ...\n");
        return 1;
    }

    Playlist playlist;
    playlist_init(&playlist);

    SearchState search;
    search_init(&search);

    for (int i = 1; i < argc; i++)
        playlist_add(&playlist, argv[i]);

    playlist_sort(&playlist);

    if (playlist.count <= 0)
    {
        media_keys_shutdown();
        input_shutdown();
        screen_shutdown();
        return 1;
    }

    PlayerState player;
    player_init(&player);

    if (!mpg_init())
    {
        media_keys_shutdown();
        input_shutdown();
        screen_shutdown();
        return 1;
    }

    MP3Player mpg = {0};
    Audio *audio = NULL;
    AudioPlayer audio_player = {0};

    if (!load_track(
            &player,
            &mpg,
            &audio_player,
            &audio,
            playlist.paths[playlist.current]))
    {
        media_keys_shutdown();
        input_shutdown();
        screen_shutdown();
        return 1;
    }

    int running = 1;
    int need_render = 1;
    long long last_switch_time = 0;
    const long switch_delay = 150;

    while (running)
    {
        Key key = input_poll();
        Action action = ACTION_NONE;
        int media_action = media_keys_poll(&action);

        /* While searching, typed letters are search text. */
        if (search.active)
        {
            if (key.type == KEY_ESCAPE)
            {
                search_stop(&search);
                need_render = 1;
            }
            else if (key.type == KEY_ENTER)
            {
                if (search.result_count > 0)
                {
                    int index = search.results[search.selected];

                    if (!start_track(
                            &player,
                            &playlist,
                            &mpg,
                            &audio_player,
                            &audio,
                            index))
                    {
                        running = 0;
                    }
                    else
                    {
                        search_stop(&search);
                        last_switch_time = now_ms();
                        need_render = 1;
                    }
                }
            }
            else if (key.type == KEY_UP)
            {
                search_select_move(&search, -1);
                need_render = 1;
            }
            else if (key.type == KEY_DOWN)
            {
                search_select_move(&search, 1);
                need_render = 1;
            }
            else if (key.type == KEY_LEFT ||
                     key.type == KEY_RIGHT)
            {
                /* Arrow keys do not leave or modify search mode. */
            }
            else if (key.type == KEY_BACKSPACE)
            {
                search_backspace(&search);
                search_update(&search, &playlist);
                need_render = 1;
            }
            else if (key.type == KEY_CHAR)
            {
                search_add_key(&search, key);
                search_update(&search, &playlist);
                need_render = 1;
            }
        }
        else
        {
            if (!media_action)
                action = action_from_key(key);

            if (action == ACTION_QUIT)
            {
                running = 0;
            }
            else if (media_action)
            {
                long long current_time = now_ms();

                if (action == ACTION_NEXT &&
                    current_time - last_switch_time >= switch_delay &&
                    playlist_next(&playlist, player.shuffle))
                {
                    if (!start_track(
                            &player, &playlist, &mpg,
                            &audio_player, &audio,
                            playlist.current))
                        running = 0;
                    else
                    {
                        last_switch_time = current_time;
                        need_render = 1;
                    }
                }
                else if (action == ACTION_BACK &&
                         current_time - last_switch_time >= switch_delay &&
                         playlist_previous(&playlist, player.shuffle))
                {
                    if (!start_track(
                            &player, &playlist, &mpg,
                            &audio_player, &audio,
                            playlist.current))
                        running = 0;
                    else
                    {
                        last_switch_time = current_time;
                        need_render = 1;
                    }
                }
                else if (action == ACTION_PAUSE)
                {
                    player.paused = !player.paused;
                    need_render = 1;
                }
            }
            else
            {
                long long current_time = now_ms();

                switch (action)
                {
                    case ACTION_PAUSE:
                        player.paused = !player.paused;
                        need_render = 1;
                        break;

                    case ACTION_SHUFFLE:
                        player.shuffle = !player.shuffle;
                        if (player.shuffle)
                            playlist_reset_history(&playlist);
                        need_render = 1;
                        break;

                    case ACTION_NEXT:
                        if (current_time - last_switch_time >= switch_delay &&
                            playlist_next(&playlist, player.shuffle))
                        {
                            if (!start_track(
                                    &player, &playlist, &mpg,
                                    &audio_player, &audio,
                                    playlist.current))
                                running = 0;
                            else
                            {
                                last_switch_time = current_time;
                                need_render = 1;
                            }
                        }
                        break;

                    case ACTION_BACK:
                        if (current_time - last_switch_time >= switch_delay &&
                            playlist_previous(&playlist, player.shuffle))
                        {
                            if (!start_track(
                                    &player, &playlist, &mpg,
                                    &audio_player, &audio,
                                    playlist.current))
                                running = 0;
                            else
                            {
                                last_switch_time = current_time;
                                need_render = 1;
                            }
                        }
                        break;

                    case ACTION_SEEK_FORWARD:
                        audio_player_seek(&audio_player, 5);
                        need_render = 1;
                        break;

                    case ACTION_SEEK_BACKWARD:
                        audio_player_seek(&audio_player, -5);
                        need_render = 1;
                        break;

                    case ACTION_SEARCH:
                        search_start(&search);
                        search_update(&search, &playlist);
                        need_render = 1;
                        break;

                    default:
                        break;
                }
            }
        }

        if (player.ended && !player.paused && !search.active)
        {
            player.ended = 0;

            if (playlist_next(&playlist, player.shuffle))
            {
                if (!start_track(
                        &player, &playlist, &mpg,
                        &audio_player, &audio,
                        playlist.current))
                    running = 0;
                else
                {
                    last_switch_time = now_ms();
                    need_render = 1;
                }
            }
            else
            {
                running = 0;
            }
        }

        long position = mpg_position_seconds(&mpg);

        if (player.position != position)
        {
            player.position = (int)position;
            need_render = 1;
        }

        if (need_render)
        {
            render_player(&player, &playlist, &search);
            need_render = 0;
        }

        sleep_ms(30);
    }

    audio_player_stop(&audio_player);
    audio_drain(audio);
    audio_destroy(audio);
    mpg_close(&mpg);

    media_keys_shutdown();
    input_shutdown();
    screen_shutdown();

    return 0;
}
