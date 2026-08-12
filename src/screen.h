#pragma once

void screen_init(void);
void screen_shutdown(void);
void screen_clear(void);
void screen_move(unsigned row, unsigned col);
void screen_write(const char *text);
void screen_clear_line(void);
void screen_get_size(int *rows, int *cols);
