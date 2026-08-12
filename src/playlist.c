#include "playlist.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "metadata.h"
#include "player.h"

static int is_mp3(const char *path)
{
    const char *dot = strrchr(path, '.');

    if (!dot)
        return 0;

    return strcmp(dot, ".mp3") == 0 ||
           strcmp(dot, ".MP3") == 0;
}

static int char_category(unsigned char c)
{
    if (c >= '0' && c <= '9') return 0;
    if ((c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z')) return 1;
    if (c == 0xD0 || c == 0xD1) return 2;
    return 3;
}

static int compare_titles(const char *a, const char *b)
{
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;

    while (*pa && *pb)
    {
        int ca = char_category(*pa);
        int cb = char_category(*pb);

        if (ca != cb)
            return ca - cb;

        if (ca == 1)
        {
            unsigned char aa = (unsigned char)tolower(*pa);
            unsigned char bb = (unsigned char)tolower(*pb);

            if (aa != bb)
                return aa - bb;
        }
        else if (*pa != *pb)
        {
            return (int)*pa - (int)*pb;
        }

        pa++;
        pb++;
    }

    if (*pa) return 1;
    if (*pb) return -1;
    return 0;
}

static void swap_tracks(Playlist *playlist, int a, int b)
{
    char temp_path[512];
    char temp_title[PLAYLIST_TITLE_SIZE];

    strcpy(temp_path, playlist->paths[a]);
    strcpy(playlist->paths[a], playlist->paths[b]);
    strcpy(playlist->paths[b], temp_path);

    strcpy(temp_title, playlist->titles[a]);
    strcpy(playlist->titles[a], playlist->titles[b]);
    strcpy(playlist->titles[b], temp_title);
}

void playlist_init(Playlist *playlist)
{
    memset(playlist, 0, sizeof(*playlist));
    playlist->history_position = -1;
}

int playlist_add(Playlist *playlist, const char *path)
{
    if (!is_mp3(path))
        return 0;

    if (playlist->count >= PLAYLIST_MAX_TRACKS)
        return 0;

    int index = playlist->count;
    PlayerState metadata;
    player_init(&metadata);

    if (!metadata_load(path, &metadata) ||
        metadata.track[0] == '\0')
    {
        const char *slash = strrchr(path, '/');
        const char *name = slash ? slash + 1 : path;

        snprintf(
            playlist->titles[index],
            PLAYLIST_TITLE_SIZE,
            "%s",
            name
        );
    }
    else
    {
        snprintf(
            playlist->titles[index],
            PLAYLIST_TITLE_SIZE,
            "%s",
            metadata.track
        );
    }

    playlist->titles[index][PLAYLIST_TITLE_SIZE - 1] = '\0';

    snprintf(
        playlist->paths[index],
        sizeof(playlist->paths[index]),
        "%s",
        path
    );

    playlist->count++;
    return 1;
}

void playlist_sort(Playlist *playlist)
{
    for (int i = 0; i < playlist->count - 1; i++)
    {
        for (int j = i + 1; j < playlist->count; j++)
        {
            int result = compare_titles(
                playlist->titles[i],
                playlist->titles[j]
            );

            if (result > 0 ||
                (result == 0 &&
                 strcmp(playlist->paths[i], playlist->paths[j]) > 0))
            {
                swap_tracks(playlist, i, j);
            }
        }
    }

    if (playlist->count > 0)
        playlist->current = 0;

    playlist_reset_history(playlist);
}

const char *playlist_current(Playlist *playlist)
{
    if (playlist->count <= 0)
        return NULL;

    return playlist->paths[playlist->current];
}

void playlist_reset_history(Playlist *playlist)
{
    if (playlist->count <= 0)
    {
        playlist->history_count = 0;
        playlist->history_position = -1;
        return;
    }

    playlist->history[0] = playlist->current;
    playlist->history_count = 1;
    playlist->history_position = 0;
}

static void history_add(Playlist *playlist, int index)
{
    if (playlist->history_position + 1 < playlist->history_count)
        playlist->history_count = playlist->history_position + 1;

    if (playlist->history_count >= PLAYLIST_MAX_TRACKS)
    {
        memmove(
            &playlist->history[0],
            &playlist->history[1],
            sizeof(int) * (PLAYLIST_MAX_TRACKS - 1)
        );
        playlist->history_count = PLAYLIST_MAX_TRACKS - 1;
    }

    playlist->history[playlist->history_count++] = index;
    playlist->history_position = playlist->history_count - 1;
}

int playlist_next(Playlist *playlist, int shuffle)
{
    if (playlist->count <= 0)
        return 0;

    if (shuffle &&
        playlist->history_position + 1 < playlist->history_count)
    {
        playlist->history_position++;
        playlist->current = playlist->history[playlist->history_position];
        return 1;
    }

    int next;

    if (shuffle)
    {
        if (playlist->count == 1)
            next = 0;
        else
        {
            do
                next = rand() % playlist->count;
            while (next == playlist->current);
        }
    }
    else
    {
        next = playlist->current + 1;
        if (next >= playlist->count)
            next = 0;
    }

    playlist->current = next;

    if (shuffle)
        history_add(playlist, next);

    return 1;
}

int playlist_previous(Playlist *playlist, int shuffle)
{
    if (playlist->count <= 0)
        return 0;

    if (shuffle)
    {
        if (playlist->history_position <= 0)
            return 0;

        playlist->history_position--;
        playlist->current = playlist->history[playlist->history_position];
        return 1;
    }

    if (playlist->current <= 0)
        return 0;

    playlist->current--;
    return 1;
}
