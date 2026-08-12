#include "screen.h"

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void write_raw(const char *text)
{
    write(STDOUT_FILENO, text, strlen(text));
}

void screen_init(void)
{
    write_raw("\033[?1049h");
    write_raw("\033[2J");
    write_raw("\033[H");
    write_raw("\033[?25l");
}

void screen_shutdown(void)
{
    write_raw("\033[?25h");
    write_raw("\033[0m");
    write_raw("\033[H");
    write_raw("\033[?1049l");
}

void screen_clear(void)
{
    write_raw("\033[2J\033[H");
}

void screen_move(unsigned row, unsigned col)
{
    char buffer[32];
    int len = snprintf(
        buffer,
        sizeof(buffer),
        "\033[%u;%uH",
        row + 1,
        col + 1
    );

    write(STDOUT_FILENO, buffer, (size_t)len);
}

void screen_write(const char *text)
{
    write(STDOUT_FILENO, text, strlen(text));
}

void screen_clear_line(void)
{
    write_raw("\033[2K");
}

void screen_get_size(int *rows, int *cols)
{
    struct winsize size;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0)
    {
        if (rows)
            *rows = size.ws_row > 0 ? size.ws_row : 24;

        if (cols)
            *cols = size.ws_col > 0 ? size.ws_col : 80;

        return;
    }

    if (rows)
        *rows = 24;

    if (cols)
        *cols = 80;
}
