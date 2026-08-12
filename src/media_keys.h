#pragma once

#include "action.h"

/*
 * Инициализация глобальных мультимедийных клавиш.
 *
 * Слушает /dev/input/event*
 * и переводит медиаклавиши в Action.
 */
int media_keys_init(void);

/*
 * Проверить, пришла ли мультимедийная клавиша.
 *
 * Возвращает 1, если действие получено.
 * Само действие записывается в *action.
 */
int media_keys_poll(Action *action);

/*
 * Остановить поток и освободить ресурсы.
 */
void media_keys_shutdown(void);
