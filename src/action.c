#include "action.h"

static int is_utf8(Key key, unsigned char a, unsigned char b)
{
    return key.length == 2 &&
           key.bytes[0] == a &&
           key.bytes[1] == b;
}

Action action_from_key(Key key)
{
    if (key.type == KEY_UP)
        return ACTION_SEARCH_UP;

    if (key.type == KEY_DOWN)
        return ACTION_SEARCH_DOWN;

    if (key.type == KEY_ENTER)
        return ACTION_SEARCH_ENTER;

    if (key.type == KEY_BACKSPACE)
        return ACTION_SEARCH_BACKSPACE;

    if (key.type != KEY_CHAR)
        return ACTION_NONE;

    switch (key.ch)
    {
        case 'q': return ACTION_QUIT;
        case 'n': return ACTION_NEXT;
        case 'b': return ACTION_BACK;
        case '>': return ACTION_SEEK_FORWARD;
        case '<': return ACTION_SEEK_BACKWARD;
        case 'p': return ACTION_PAUSE;
        case 's': return ACTION_SHUFFLE;
        case '/': return ACTION_SEARCH;

        /* Russian keyboard equivalents. */
        default: break;
    }

    if (is_utf8(key, 0xD0, 0xB9)) return ACTION_QUIT;      /* й */
    if (is_utf8(key, 0xD1, 0x82)) return ACTION_NEXT;      /* т */
    if (is_utf8(key, 0xD0, 0xB8)) return ACTION_BACK;      /* и */
    if (is_utf8(key, 0xD0, 0xB7)) return ACTION_PAUSE;     /* з */
    if (is_utf8(key, 0xD1, 0x8B)) return ACTION_SHUFFLE;   /* ы */

    return ACTION_NONE;
}
