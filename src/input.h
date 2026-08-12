#pragma once

typedef enum
{
    KEY_NONE = 0,
    KEY_CHAR,
    KEY_ESCAPE,

    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER,
    KEY_BACKSPACE
} KeyType;

typedef struct
{
    KeyType type;
    char ch;
    unsigned char bytes[4];
    int length;
} Key;

int input_init(void);
void input_shutdown(void);

Key input_poll(void);
void input_flush(void);
