#define _GNU_SOURCE

#include "media_keys.h"

#include <linux/input.h>

#include <fcntl.h>
#include <pthread.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#define MAX_INPUT_DEVICES 64

typedef struct
{
int fd;
char name[256];
} InputDevice;

static InputDevice devices[MAX_INPUT_DEVICES];
static int device_count = 0;

static int pipe_fds[2] = {-1, -1};

static pthread_t thread;
static int running = 0;

/*

* ---
* Преобразование Linux media key -> Action
* ---

*/

static Action action_from_media_key(
unsigned short code
)
{
switch (code)
{
case KEY_NEXTSONG:
return ACTION_NEXT;

    case KEY_PREVIOUSSONG:
        return ACTION_BACK;

    /*
     * JBL Tune 720BT обычно использует
     * KEY_PAUSECD для кнопки play/pause.
     *
     * Некоторые устройства используют
     * KEY_PLAYPAUSE или KEY_PLAY.
     */
    case KEY_PLAYPAUSE:
    case KEY_PAUSECD:
    case KEY_PLAY:
    case KEY_PLAYCD:
        return ACTION_PAUSE;

    default:
        return ACTION_NONE;
}
}

/*

* ---
* Закрытие устройств
* ---

*/

static void close_devices(void)
{
for (int i = 0; i < device_count; i++)
{
if (devices[i].fd >= 0)
{
close(devices[i].fd);
devices[i].fd = -1;
}
}
device_count = 0;
}

/*

* ---
* Открытие input devices
* ---

*/

static void open_input_devices(void)
{
close_devices();
for (int i = 0; i < MAX_INPUT_DEVICES; i++)
{
    char path[64];

    snprintf(
        path,
        sizeof(path),
        "/dev/input/event%d",
        i
    );

    int fd = open(
        path,
        O_RDONLY | O_NONBLOCK
    );

    if (fd < 0)
        continue;

    char name[256];

    memset(
        name,
        0,
        sizeof(name)
    );

    if (ioctl(
            fd,
            EVIOCGNAME(sizeof(name)),
            name
        ) < 0)
    {
        close(fd);
        continue;
    }

    if (device_count >= MAX_INPUT_DEVICES)
    {
        close(fd);
        break;
    }

    devices[device_count].fd = fd;

    snprintf(
        devices[device_count].name,
        sizeof(devices[device_count].name),
        "%s",
        name
    );

    devices[device_count]
        .name[
            sizeof(devices[device_count].name) - 1
        ] = '\0';

    device_count++;
}
}

/*

* ---
* Передача Action в главный поток
* ---

*/

static void send_action(Action action)
{
if (action == ACTION_NONE)
return;
if (pipe_fds[1] < 0)
    return;

/*
 * PIPE_BUF гарантирует атомарность такой
 * маленькой записи.
 */
ssize_t result = write(
    pipe_fds[1],
    &action,
    sizeof(action)
);

(void)result;
}

/*

* ---
* Поток обработки Linux input
* ---

*/

static void *media_thread(void *unused)
{
(void)unused;
open_input_devices();

while (running)
{
    if (device_count == 0)
    {
        usleep(500000);

        if (running)
            open_input_devices();

        continue;
    }

    struct pollfd pollfds[MAX_INPUT_DEVICES];

    for (int i = 0; i < device_count; i++)
    {
        pollfds[i].fd =
            devices[i].fd;

        pollfds[i].events =
            POLLIN;

        pollfds[i].revents =
            0;
    }

    int result = poll(
        pollfds,
        device_count,
        500
    );

    if (result < 0)
    {
        if (errno == EINTR)
            continue;

        break;
    }

    if (result == 0)
        continue;


    for (int i = 0; i < device_count; i++)
    {
        if (!(pollfds[i].revents & POLLIN))
            continue;


        struct input_event event;


        while (running)
        {
            ssize_t size =
                read(
                    devices[i].fd,
                    &event,
                    sizeof(event)
                );


            if (size < 0)
            {
                if (
                    errno == EAGAIN ||
                    errno == EWOULDBLOCK
                )
                {
                    break;
                }

                break;
            }


            if (size != sizeof(event))
                break;


            /*
             * Нас интересуют только клавиши.
             */
            if (event.type != EV_KEY)
                continue;


            /*
             * 0 = release
             * 1 = press
             * 2 = repeat
             *
             * Обрабатываем ТОЛЬКО press.
             */
            if (event.value != 1)
                continue;


            Action action =
                action_from_media_key(
                    event.code
                );


            if (action == ACTION_NONE)
                continue;


            /*
             * Одно физическое нажатие =
             * одно Action.
             */
            send_action(action);
        }
    }
}


close_devices();

return NULL;
}

/*

* ---
* Init
* ---

*/

int media_keys_init(void)
{
if (running)
return 1;
if (pipe(pipe_fds) < 0)
    return 0;


int flags =
    fcntl(
        pipe_fds[0],
        F_GETFL,
        0
    );


if (flags >= 0)
{
    fcntl(
        pipe_fds[0],
        F_SETFL,
        flags | O_NONBLOCK
    );
}


running = 1;


if (pthread_create(
        &thread,
        NULL,
        media_thread,
        NULL
    ) != 0)
{
    running = 0;

    close(pipe_fds[0]);
    close(pipe_fds[1]);

    pipe_fds[0] = -1;
    pipe_fds[1] = -1;

    return 0;
}


return 1;
}

/*

* ---
* Получение Action главным потоком
* ---

*/

int media_keys_poll(Action *action)
{
if (action == NULL)
return 0;
*action = ACTION_NONE;


if (pipe_fds[0] < 0)
    return 0;


/*
 * Получаем одно действие.
 *
 * sizeof(Action) гарантированно записывается
 * целиком через pipe.
 */
ssize_t size =
    read(
        pipe_fds[0],
        action,
        sizeof(*action)
    );


if (size != sizeof(*action))
{
    *action = ACTION_NONE;
    return 0;
}


return 1;
}

/*

* ---
* Shutdown
* ---

*/

void media_keys_shutdown(void)
{
if (!running)
return;
running = 0;


pthread_join(
    thread,
    NULL
);


if (pipe_fds[0] >= 0)
    close(pipe_fds[0]);


if (pipe_fds[1] >= 0)
    close(pipe_fds[1]);


pipe_fds[0] = -1;
pipe_fds[1] = -1;

}

