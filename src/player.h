#pragma once

typedef struct
{
    char path[512];

    char track[256];
    char artist[256];
    char album[256];

    char cover[512];

    int duration;
    int position;

    int paused;
    int shuffle;
    int ended;
} PlayerState;

void player_init(PlayerState *player);
