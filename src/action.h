#pragma once

#include "input.h"

typedef enum
{
    ACTION_NONE = 0,

    ACTION_NEXT,
    ACTION_BACK,

    ACTION_SEEK_FORWARD,
    ACTION_SEEK_BACKWARD,

    ACTION_PAUSE,
    ACTION_SHUFFLE,
    ACTION_SEARCH,

    ACTION_SEARCH_UP,
    ACTION_SEARCH_DOWN,
    ACTION_SEARCH_ENTER,
    ACTION_SEARCH_BACKSPACE,

    ACTION_QUIT
} Action;

Action action_from_key(Key key);
