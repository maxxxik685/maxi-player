#include "player.h"

#include <string.h>

void player_init(PlayerState *player)
{
    memset(player, 0, sizeof(*player));

    strcpy(player->track, "Test Song");
    strcpy(player->artist, "Max");

    player->duration = 238;
    player->position = 0;
    player->paused = 0;
    player->shuffle = 1;
    player->ended = 0;
}
