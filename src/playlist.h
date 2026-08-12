#pragma once

#define PLAYLIST_MAX_TRACKS 1024
#define PLAYLIST_TITLE_SIZE 256

typedef struct
{
    char paths[PLAYLIST_MAX_TRACKS][512];
    char titles[PLAYLIST_MAX_TRACKS][PLAYLIST_TITLE_SIZE];

    int count;
    int current;

    int history[PLAYLIST_MAX_TRACKS];
    int history_count;
    int history_position;
} Playlist;

void playlist_init(Playlist *playlist);
int playlist_add(Playlist *playlist, const char *path);
const char *playlist_current(Playlist *playlist);
void playlist_sort(Playlist *playlist);
void playlist_reset_history(Playlist *playlist);
int playlist_next(Playlist *playlist, int shuffle);
int playlist_previous(Playlist *playlist, int shuffle);
