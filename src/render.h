#pragma once

#include "player.h"
#include "playlist.h"
#include "search.h"

void render_player(
    PlayerState *player,
    Playlist *playlist,
    SearchState *search
);
