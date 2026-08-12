#include "render.h"
#include "screen.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

/*
 * Terminal rendering is done as one complete frame.
 *
 * The old renderer mixed many small write() calls with cursor moves and
 * erase-line sequences. Even though the screen was not explicitly cleared,
 * the terminal could display intermediate states when the writes were
 * processed separately. Building one frame and sending it with a single
 * write() makes the update effectively atomic from the terminal's point of
 * view and removes visible tearing/flicker.
 */
#define FRAME_BUFFER_SIZE 65536

static void render_time(int seconds, char *buffer, size_t size)
{
    if (seconds < 0)
        seconds = 0;

    snprintf(buffer, size, "%02d:%02d", seconds / 60, seconds % 60);
}

static int append_text(
    char *buffer,
    size_t capacity,
    size_t *length,
    const char *text
)
{
    size_t text_length = strlen(text);

    if (*length + text_length >= capacity)
        return 0;

    memcpy(buffer + *length, text, text_length);
    *length += text_length;
    buffer[*length] = '\0';
    return 1;
}

static int append_format(
    char *buffer,
    size_t capacity,
    size_t *length,
    const char *format,
    ...
)
{
    va_list args;
    va_start(args, format);

    int written = vsnprintf(
        buffer + *length,
        capacity - *length,
        format,
        args
    );

    va_end(args);

    if (written < 0 || (size_t)written >= capacity - *length)
        return 0;

    *length += (size_t)written;
    return 1;
}

static int append_line_start(
    char *buffer,
    size_t capacity,
    size_t *length,
    int row
)
{
    /* Home + row positioning is avoided; just position explicitly. */
    return append_format(
        buffer,
        capacity,
        length,
        "\033[%d;1H\033[2K",
        row + 1
    );
}

static int append_status(
    char *buffer,
    size_t capacity,
    size_t *length,
    const PlayerState *player
)
{
    if (player->shuffle)
    {
        if (!append_text(buffer, capacity, length, "SHUFFLE"))
            return 0;
    }
    else if (!append_text(buffer, capacity, length, "        "))
    {
        return 0;
    }

    if (player->paused)
    {
        if (!append_text(buffer, capacity, length, " PAUSE"))
            return 0;
    }

    return 1;
}

static int append_progress(
    char *buffer,
    size_t capacity,
    size_t *length,
    const PlayerState *player
)
{
    const int bar_size = 20;
    int filled = 0;

    if (player->duration > 0)
    {
        filled = player->position * bar_size / player->duration;

        if (filled < 0)
            filled = 0;
        if (filled > bar_size)
            filled = bar_size;
    }

    if (!append_text(buffer, capacity, length, "["))
        return 0;

    for (int i = 0; i < bar_size; i++)
    {
        if (!append_text(
                buffer,
                capacity,
                length,
                i < filled ? "#" : "-"))
        {
            return 0;
        }
    }

    if (!append_text(buffer, capacity, length, "]"))
        return 0;

    int percent = 0;
    if (player->duration > 0)
        percent = player->position * 100 / player->duration;

    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    return append_format(
        buffer,
        capacity,
        length,
        " %d%%",
        percent
    );
}

static int append_search(
    char *buffer,
    size_t capacity,
    size_t *length,
    const Playlist *playlist,
    const SearchState *search,
    int search_row,
    int rows
)
{
    if (rows <= search_row)
        return 1;

    int visible = rows - search_row - 1;
    if (visible < 0)
        visible = 0;

    int start = 0;
    int end = 0;

    if (search->active && search->result_count > 0 && visible > 0)
    {
        if (search->selected >= visible)
            start = search->selected - visible + 1;

        end = start + visible;
        if (end > search->result_count)
            end = search->result_count;
    }

    /* Prompt. */
    if (!append_line_start(buffer, capacity, length, search_row))
        return 0;

    if (search->active &&
        !append_format(buffer, capacity, length, "/%s", search->text))
    {
        return 0;
    }

    /* Clear and draw the complete search area. */
    for (int row = search_row + 1; row < rows; row++)
    {
        if (!append_line_start(buffer, capacity, length, row))
            return 0;

        int result_index = row - (search_row + 1) + start;
        if (result_index < start || result_index >= end)
            continue;

        int index = search->results[result_index];
        const char *title = playlist->titles[index];

        if (result_index == search->selected)
        {
            if (!append_format(
                    buffer,
                    capacity,
                    length,
                    "\033[7m%s\033[27m",
                    title))
            {
                return 0;
            }
        }
        else if (!append_text(buffer, capacity, length, title))
        {
            return 0;
        }
    }

    return 1;
}

void render_player(
    PlayerState *player,
    Playlist *playlist,
    SearchState *search
)
{
    int rows;
    int cols;
    screen_get_size(&rows, &cols);
    (void)cols;

    if (rows < 8)
        rows = 8;

    char frame[FRAME_BUFFER_SIZE];
    size_t length = 0;

    /* Start at the first row and clear every row as it is redrawn. */
    if (!append_text(frame, sizeof(frame), &length, "\033[H"))
        return;

    if (!append_line_start(frame, sizeof(frame), &length, 0))
        return;
    if (!append_text(frame, sizeof(frame), &length, "Title:  "))
        return;
    if (!append_text(frame, sizeof(frame), &length, player->track))
        return;

    if (!append_line_start(frame, sizeof(frame), &length, 1))
        return;
    if (!append_text(frame, sizeof(frame), &length, "Artist: "))
        return;
    if (!append_text(frame, sizeof(frame), &length, player->artist))
        return;

    if (!append_line_start(frame, sizeof(frame), &length, 2))
        return;
    if (!append_progress(frame, sizeof(frame), &length, player))
        return;

    if (!append_line_start(frame, sizeof(frame), &length, 3))
        return;

    char current[16];
    char total[16];
    render_time(player->position, current, sizeof(current));
    render_time(player->duration, total, sizeof(total));

    if (!append_format(
            frame,
            sizeof(frame),
            &length,
            "%s / %s",
            current,
            total))
    {
        return;
    }

    if (!append_line_start(frame, sizeof(frame), &length, 4))
        return;
    if (!append_status(frame, sizeof(frame), &length, player))
        return;

    if (!append_search(
            frame,
            sizeof(frame),
            &length,
            playlist,
            search,
            5,
            rows))
    {
        return;
    }

    /* One syscall per frame: no partially rendered frames are exposed. */
    (void)write(STDOUT_FILENO, frame, length);
}
