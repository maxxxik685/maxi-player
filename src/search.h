#pragma once

#include "playlist.h"

#define SEARCH_TEXT_SIZE 256
#define SEARCH_RESULTS PLAYLIST_MAX_TRACKS

typedef struct
{
    char text[SEARCH_TEXT_SIZE];

    int active;
    int length;

    int results[SEARCH_RESULTS];
    int result_count;
    int selected;
} SearchState;

void search_init(SearchState *search);
void search_start(SearchState *search);
void search_stop(SearchState *search);
void search_clear(SearchState *search);

int search_add_byte(
    SearchState *search,
    unsigned char byte
);

void search_backspace(SearchState *search);
void search_update(SearchState *search, Playlist *playlist);
