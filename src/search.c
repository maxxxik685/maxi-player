#include "search.h"

#include <ctype.h>
#include <string.h>

/*
 * Поиск:
 *
 * - Регистр не важен.
 * - Раскладка важна:
 *      "ram" -> только латиница
 *      "рам" -> только кириллица
 * - Сначала идут названия, начинающиеся с запроса.
 * - Затем названия, где запрос встречается раньше.
 * - Самый вероятный результат находится первым,
 *   сразу под строкой поиска.
 */

/* --------------------------------------------------------- */
/* ASCII                                                     */
/* --------------------------------------------------------- */

static unsigned char ascii_lower(unsigned char c)
{
    if (c >= 'A' && c <= 'Z')
        return (unsigned char)(c + ('a' - 'A'));

    return c;
}

/* --------------------------------------------------------- */
/* UTF-8                                                     */
/* --------------------------------------------------------- */

static int utf8_char_length(unsigned char c)
{
    if (c < 0x80)
        return 1;

    if ((c & 0xE0) == 0xC0)
        return 2;

    if ((c & 0xF0) == 0xE0)
        return 3;

    if ((c & 0xF8) == 0xF0)
        return 4;

    return 1;
}

/*
 * Приводит одну кириллическую букву к нижнему регистру.
 *
 * Возвращает количество байт UTF-8:
 * 2 = кириллица
 * 0 = не кириллица
 */
static int utf8_lower_char(
    const unsigned char *src,
    unsigned char *dst
)
{
    unsigned char a = src[0];
    unsigned char b = src[1];

    /*
     * А-П -> а-п
     */
    if (a == 0xD0 &&
        b >= 0x90 &&
        b <= 0x9F)
    {
        dst[0] = 0xD0;
        dst[1] = (unsigned char)(b + 0x20);

        return 2;
    }

    /*
     * Р-Я -> р-я
     */
    if (a == 0xD0 &&
        b >= 0xA0 &&
        b <= 0xAF)
    {
        dst[0] = 0xD1;
        dst[1] = (unsigned char)(b - 0x20);

        return 2;
    }

    /*
     * Ё -> ё
     */
    if (a == 0xD0 &&
        b == 0x81)
    {
        dst[0] = 0xD1;
        dst[1] = 0x91;

        return 2;
    }

    /*
     * Уже строчная кириллица.
     */
    if (a == 0xD0 &&
        b >= 0xB0 &&
        b <= 0xBF)
    {
        dst[0] = a;
        dst[1] = b;

        return 2;
    }

    if (a == 0xD1 &&
        b >= 0x80 &&
        b <= 0x8F)
    {
        dst[0] = a;
        dst[1] = b;

        return 2;
    }

    /*
     * ё
     */
    if (a == 0xD1 &&
        b == 0x91)
    {
        dst[0] = a;
        dst[1] = b;

        return 2;
    }

    return 0;
}

/*
 * Сравнение одного UTF-8 символа
 * без учёта регистра.
 *
 * При этом русский и английский
 * никогда не считаются одинаковыми.
 */
static int utf8_char_equal(
    const unsigned char *a,
    const unsigned char *b
)
{
    /*
     * ASCII.
     */
    if (*a < 0x80 &&
        *b < 0x80)
    {
        return ascii_lower(*a) ==
               ascii_lower(*b);
    }

    /*
     * Кириллица.
     */
    if (*a >= 0x80 &&
        *b >= 0x80)
    {
        unsigned char la[4];
        unsigned char lb[4];

        int lena =
            utf8_lower_char(a, la);

        int lenb =
            utf8_lower_char(b, lb);

        if (lena > 0 &&
            lenb > 0)
        {
            if (lena != lenb)
                return 0;

            for (int i = 0; i < lena; i++)
            {
                if (la[i] != lb[i])
                    return 0;
            }

            return 1;
        }
    }

    return 0;
}

/* --------------------------------------------------------- */
/* Поиск совпадения                                          */
/* --------------------------------------------------------- */

/*
 * Возвращает позицию совпадения в UTF-8 символах.
 *
 * 0  = название начинается с запроса
 * 1+ = запрос найден внутри
 * -1 = не найден
 */
static int find_match(
    const char *title,
    const char *query
)
{
    if (!*query)
        return -1;

    const unsigned char *start =
        (const unsigned char *)title;

    int character_position = 0;

    while (*start)
    {
        const unsigned char *a = start;
        const unsigned char *b =
            (const unsigned char *)query;

        while (*a && *b)
        {
            int lena =
                utf8_char_length(*a);

            int lenb =
                utf8_char_length(*b);

            if (lena != lenb)
                break;

            if (!utf8_char_equal(a, b))
                break;

            a += lena;
            b += lenb;
        }

        /*
         * Весь запрос совпал.
         */
        if (!*b)
            return character_position;

        start +=
            utf8_char_length(*start);

        character_position++;
    }

    return -1;
}

/* --------------------------------------------------------- */
/* Состояние поиска                                          */
/* --------------------------------------------------------- */

void search_init(
    SearchState *search
)
{
    memset(
        search,
        0,
        sizeof(*search)
    );

    search->active = 0;
    search->length = 0;
    search->result_count = 0;
    search->selected = 0;
}

void search_start(
    SearchState *search
)
{
    search->active = 1;
    search->length = 0;
    search->text[0] = '\0';

    search->result_count = 0;
    search->selected = 0;
}

void search_stop(
    SearchState *search
)
{
    search->active = 0;
    search->length = 0;
    search->text[0] = '\0';

    search->result_count = 0;
    search->selected = 0;
}

void search_clear(
    SearchState *search
)
{
    search->length = 0;
    search->text[0] = '\0';

    search->result_count = 0;
    search->selected = 0;
}

/* --------------------------------------------------------- */
/* Ввод                                                      */
/* --------------------------------------------------------- */

int search_add_byte(
    SearchState *search,
    unsigned char byte
)
{
    if (!search->active)
        return 0;

    if (byte < 32)
        return 0;

    if (byte == 127)
        return 0;

    if (search->length >=
        SEARCH_TEXT_SIZE - 1)
    {
        return 0;
    }

    search->text[
        search->length
    ] = (char)byte;

    search->length++;

    search->text[
        search->length
    ] = '\0';

    return 1;
}

void search_backspace(
    SearchState *search
)
{
    if (!search->active)
        return;

    if (search->length <= 0)
        return;

    int pos =
        search->length - 1;

    /*
     * Удаляем весь последний
     * UTF-8 символ.
     */
    while (pos > 0 &&
           ((unsigned char)
            search->text[pos] & 0xC0) == 0x80)
    {
        pos--;
    }

    search->length = pos;
    search->text[pos] = '\0';

    search->selected = 0;
}

/* --------------------------------------------------------- */
/* Результаты поиска                                         */
/* --------------------------------------------------------- */

typedef struct
{
    int index;
    int position;
} SearchMatch;

/*
 * Сравнение результатов.
 *
 * Чем меньше position,
 * тем лучше результат.
 *
 * position == 0:
 * название начинается с запроса.
 *
 * После этого идут совпадения
 * внутри названия.
 */
static int match_better(
    SearchMatch a,
    SearchMatch b
)
{
    int a_starts =
        a.position == 0;

    int b_starts =
        b.position == 0;

    if (a_starts != b_starts)
        return a_starts > b_starts;

    if (a.position != b.position)
        return a.position < b.position;

    /*
     * Если совпадение одинаковое —
     * сохраняем порядок плейлиста.
     */
    return a.index < b.index;
}

void search_update(
    SearchState *search,
    Playlist *playlist
)
{
    if (!search->active)
        return;

    search->result_count = 0;
    search->selected = 0;

    if (search->length <= 0)
    {
        for (int i = 0; i < playlist->count; i++)
            search->results[i] = i;

        search->result_count = playlist->count;
        return;
    }

    SearchMatch matches[
        PLAYLIST_MAX_TRACKS
    ];

    int count = 0;

    /*
     * Ищем совпадения во всём плейлисте.
     */
    for (int i = 0;
         i < playlist->count;
         i++)
    {
        int position =
            find_match(
                playlist->titles[i],
                search->text
            );

        if (position < 0)
            continue;

        matches[count].index = i;
        matches[count].position = position;

        count++;
    }

    /*
     * Сортируем от самого вероятного
     * к самому слабому.
     */
    for (int i = 0;
         i < count - 1;
         i++)
    {
        for (int j = i + 1;
             j < count;
             j++)
        {
            if (match_better(
                    matches[j],
                    matches[i]))
            {
                SearchMatch temp =
                    matches[i];

                matches[i] =
                    matches[j];

                matches[j] =
                    temp;
            }
        }
    }

    int limit = count;

    /*
     * results[0] теперь ВСЕГДА
     * самый вероятный результат.
     */
    for (int i = 0;
         i < limit;
         i++)
    {
        search->results[i] =
            matches[i].index;
    }

    search->result_count = limit;

    /*
     * Выбран именно первый результат —
     * самый вероятный.
     */
    search->selected = 0;
}
