#include "input.h"

#include <poll.h>
#include <termios.h>
#include <unistd.h>

static struct termios old_terminal;

int input_init(void)
{
    if (tcgetattr(STDIN_FILENO, &old_terminal) != 0)
        return 0;

    struct termios raw = old_terminal;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    return tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
}

void input_shutdown(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_terminal);
}

static Key key_none(void)
{
    Key key = {0};
    key.type = KEY_NONE;
    return key;
}

static int utf8_length(unsigned char c)
{
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static int read_with_timeout(unsigned char *c, int timeout_ms)
{
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int result = poll(&pfd, 1, timeout_ms);
    if (result <= 0 || !(pfd.revents & POLLIN))
        return 0;

    return read(STDIN_FILENO, c, 1) == 1;
}

Key input_poll(void)
{
    Key key = key_none();
    unsigned char c;

    if (read(STDIN_FILENO, &c, 1) != 1)
        return key;

    if (c == 27)
    {
        unsigned char a;
        unsigned char b;

        if (!read_with_timeout(&a, 10))
        {
            key.type = KEY_ESCAPE;
            return key;
        }

        if (a != '[' || !read_with_timeout(&b, 10))
        {
            key.type = KEY_ESCAPE;
            return key;
        }

        if (b == 'A') key.type = KEY_UP;
        else if (b == 'B') key.type = KEY_DOWN;
        else if (b == 'C') key.type = KEY_RIGHT;
        else if (b == 'D') key.type = KEY_LEFT;
        else key.type = KEY_ESCAPE;

        return key;
    }

    if (c == '\n' || c == '\r')
    {
        key.type = KEY_ENTER;
        return key;
    }

    if (c == 127 || c == 8)
    {
        key.type = KEY_BACKSPACE;
        return key;
    }

    key.type = KEY_CHAR;
    key.bytes[0] = c;
    key.length = utf8_length(c);
    key.ch = (char)c;

    if (key.length > 1)
    {
        for (int i = 1; i < key.length; i++)
        {
            if (!read_with_timeout(&key.bytes[i], 10))
            {
                key.length = 1;
                break;
            }
        }
    }

    return key;
}

void input_flush(void)
{
    tcflush(STDIN_FILENO, TCIFLUSH);
}
