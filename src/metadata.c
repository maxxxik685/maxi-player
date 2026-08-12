#include "metadata.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


static uint32_t read_be32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}


static uint32_t read_synchsafe(const unsigned char *p)
{
    return ((uint32_t)(p[0] & 0x7f) << 21) |
           ((uint32_t)(p[1] & 0x7f) << 14) |
           ((uint32_t)(p[2] & 0x7f) << 7) |
           (uint32_t)(p[3] & 0x7f);
}


static void trim(char *str)
{
    size_t len = strlen(str);

    while (len > 0)
    {
        unsigned char c = (unsigned char)str[len - 1];

        if (c != ' ' && c != '\0' && c != '\r' && c != '\n')
            break;

        str[len - 1] = '\0';
        len--;
    }
}


static int utf8_write(
    uint32_t unicode,
    char *dst,
    int max,
    int *pos
)
{
    int out = *pos;

    if (unicode <= 0x7f)
    {
        if (out + 1 >= max)
            return 0;

        dst[out++] = (char)unicode;
    }
    else if (unicode <= 0x7ff)
    {
        if (out + 2 >= max)
            return 0;

        dst[out++] =
            (char)(0xc0 | (unicode >> 6));

        dst[out++] =
            (char)(0x80 | (unicode & 0x3f));
    }
    else if (unicode <= 0xffff)
    {
        if (out + 3 >= max)
            return 0;

        dst[out++] =
            (char)(0xe0 | (unicode >> 12));

        dst[out++] =
            (char)(0x80 | ((unicode >> 6) & 0x3f));

        dst[out++] =
            (char)(0x80 | (unicode & 0x3f));
    }
    else if (unicode <= 0x10ffff)
    {
        if (out + 4 >= max)
            return 0;

        dst[out++] =
            (char)(0xf0 | (unicode >> 18));

        dst[out++] =
            (char)(0x80 | ((unicode >> 12) & 0x3f));

        dst[out++] =
            (char)(0x80 | ((unicode >> 6) & 0x3f));

        dst[out++] =
            (char)(0x80 | (unicode & 0x3f));
    }
    else
    {
        return 0;
    }

    *pos = out;

    return 1;
}


static void cp1251_to_utf8(
    const unsigned char *src,
    int size,
    char *dst,
    int max
)
{
    static const uint16_t table[32] =
    {
        0x0402, 0x0403, 0x201A, 0x0453,
        0x201E, 0x2026, 0x2020, 0x2021,
        0x20AC, 0x2030, 0x0409, 0x2039,
        0x040A, 0x040C, 0x040B, 0x040F,
        0x0452, 0x2018, 0x2019, 0x201C,
        0x201D, 0x2022, 0x2013, 0x2014,
        0x0098, 0x2122, 0x0459, 0x203A,
        0x045A, 0x045C, 0x045B, 0x045F
    };

    int out = 0;

    for (int i = 0; i < size && out < max - 1; i++)
    {
        unsigned char c = src[i];

        if (c == '\0')
            break;

        uint32_t unicode;

        if (c < 0x80)
        {
            unicode = c;
        }
        else if (c >= 0xC0)
        {
            unicode =
                0x0410 + (uint32_t)(c - 0xC0);
        }
        else if (c >= 0x80 && c <= 0xBF)
        {
            if (c <= 0x9F)
            {
                unicode = table[c - 0x80];
            }
            else
            {
                unicode =
                    0x00A0 + (uint32_t)(c - 0xA0);
            }
        }
        else
        {
            unicode = c;
        }

        if (!utf8_write(
                unicode,
                dst,
                max,
                &out))
        {
            break;
        }
    }

    dst[out] = '\0';

    trim(dst);
}


static void utf16_to_utf8(
    const unsigned char *src,
    int size,
    char *dst,
    int max
)
{
    int out = 0;

    if (size < 2)
    {
        dst[0] = '\0';
        return;
    }

    int little_endian = 0;
    int offset = 0;

    /*
     * UTF-16 BOM:
     *
     * FF FE = little endian
     * FE FF = big endian
     */

    if (src[0] == 0xff && src[1] == 0xfe)
    {
        little_endian = 1;
        offset = 2;
    }
    else if (src[0] == 0xfe && src[1] == 0xff)
    {
        little_endian = 0;
        offset = 2;
    }

    for (
        int i = offset;
        i + 1 < size;
        i += 2
    )
    {
        uint16_t value;

        if (little_endian)
        {
            value =
                (uint16_t)src[i] |
                ((uint16_t)src[i + 1] << 8);
        }
        else
        {
            value =
                ((uint16_t)src[i] << 8) |
                (uint16_t)src[i + 1];
        }

        if (value == 0)
            break;

        /*
         * UTF-16 surrogate pair.
         */

        if (
            value >= 0xD800 &&
            value <= 0xDBFF &&
            i + 3 < size
        )
        {
            uint16_t low;

            if (little_endian)
            {
                low =
                    (uint16_t)src[i + 2] |
                    ((uint16_t)src[i + 3] << 8);
            }
            else
            {
                low =
                    ((uint16_t)src[i + 2] << 8) |
                    (uint16_t)src[i + 3];
            }

            if (
                low >= 0xDC00 &&
                low <= 0xDFFF
            )
            {
                uint32_t unicode =
                    0x10000 +
                    (((uint32_t)value - 0xD800) << 10) +
                    ((uint32_t)low - 0xDC00);

                if (!utf8_write(
                        unicode,
                        dst,
                        max,
                        &out))
                {
                    break;
                }

                i += 2;
                continue;
            }
        }

        if (!utf8_write(
                value,
                dst,
                max,
                &out))
        {
            break;
        }
    }

    dst[out] = '\0';

    trim(dst);
}


static void iso88591_to_utf8(
    const unsigned char *src,
    int size,
    char *dst,
    int max
)
{
    int out = 0;

    for (int i = 0; i < size; i++)
    {
        unsigned char c = src[i];

        if (c == '\0')
            break;

        if (!utf8_write(
                c,
                dst,
                max,
                &out))
        {
            break;
        }
    }

    dst[out] = '\0';

    trim(dst);
}


static void read_text_frame(
    const unsigned char *data,
    int size,
    char *out,
    int max
)
{
    out[0] = '\0';

    if (!data || size <= 1)
        return;

    unsigned char encoding = data[0];

    const unsigned char *text =
        data + 1;

    int text_size =
        size - 1;

    switch (encoding)
    {
        /*
         * ISO-8859-1
         */
        case 0:

            iso88591_to_utf8(
                text,
                text_size,
                out,
                max
            );

            break;


        /*
         * UTF-16 with BOM
         */
        case 1:

            utf16_to_utf8(
                text,
                text_size,
                out,
                max
            );

            break;


        /*
         * UTF-16BE without BOM
         */
        case 2:
        {
            unsigned char *buffer =
                malloc((size_t)text_size + 2);

            if (!buffer)
                return;

            /*
             * Add a fake big-endian BOM.
             */
            buffer[0] = 0xfe;
            buffer[1] = 0xff;

            memcpy(
                buffer + 2,
                text,
                (size_t)text_size
            );

            utf16_to_utf8(
                buffer,
                text_size + 2,
                out,
                max
            );

            free(buffer);

            break;
        }


        /*
         * UTF-8
         */
        case 3:
        {
            int len = text_size;

            if (len >= max)
                len = max - 1;

            memcpy(
                out,
                text,
                (size_t)len
            );

            out[len] = '\0';

            trim(out);

            break;
        }


        default:
            break;
    }
}


static int read_id3v2(
    const char *path,
    PlayerState *player
)
{
    FILE *file =
        fopen(path, "rb");

    if (!file)
        return 0;


    unsigned char header[10];

    if (
        fread(header, 1, 10, file) != 10
    )
    {
        fclose(file);
        return 0;
    }


    if (
        memcmp(header, "ID3", 3) != 0
    )
    {
        fclose(file);
        return 0;
    }


    int version =
        header[3];


    uint32_t tag_size =
        read_synchsafe(&header[6]);


    /*
     * Не даём повреждённому файлу
     * выделить гигантский буфер.
     */

    if (tag_size > 16 * 1024 * 1024)
    {
        fclose(file);
        return 0;
    }


    unsigned char *tag =
        malloc(tag_size);

    if (!tag)
    {
        fclose(file);
        return 0;
    }


    if (
        fread(
            tag,
            1,
            tag_size,
            file
        ) != tag_size
    )
    {
        free(tag);
        fclose(file);
        return 0;
    }


    fclose(file);


    uint32_t position = 0;

    int found = 0;


    while (position + 10 <= tag_size)
    {
        unsigned char *frame =
            tag + position;


        /*
         * Пустой frame означает конец
         * списка frames.
         */

        if (frame[0] == 0)
            break;


        char id[5];

        memcpy(
            id,
            frame,
            4
        );

        id[4] = '\0';


        /*
         * ID3v2.4:
         * размер frame — synchsafe.
         *
         * ID3v2.3:
         * размер frame — обычный BE32.
         */

        uint32_t frame_size;

        if (version >= 4)
        {
            frame_size =
                read_synchsafe(
                    &frame[4]
                );
        }
        else
        {
            frame_size =
                read_be32(
                    &frame[4]
                );
        }


        /*
         * Защита от повреждённого ID3.
         */

        if (
            frame_size >
            tag_size - position - 10
        )
        {
            break;
        }


        unsigned char *data =
            frame + 10;


        if (strcmp(id, "TIT2") == 0)
        {
            read_text_frame(
                data,
                (int)frame_size,
                player->track,
                sizeof(player->track)
            );

            if (player->track[0] != '\0')
                found = 1;
        }
        else if (strcmp(id, "TPE1") == 0)
        {
            read_text_frame(
                data,
                (int)frame_size,
                player->artist,
                sizeof(player->artist)
            );

            if (player->artist[0] != '\0')
                found = 1;
        }
        else if (strcmp(id, "TALB") == 0)
        {
            read_text_frame(
                data,
                (int)frame_size,
                player->album,
                sizeof(player->album)
            );
        }


        position +=
            10 + frame_size;
    }


    free(tag);

    return found;
}


static int read_id3v1(
    const char *path,
    PlayerState *player
)
{
    FILE *file =
        fopen(path, "rb");

    if (!file)
        return 0;


    if (
        fseek(
            file,
            -128,
            SEEK_END
        ) != 0
    )
    {
        fclose(file);
        return 0;
    }


    unsigned char tag[128];


    if (
        fread(
            tag,
            1,
            128,
            file
        ) != 128
    )
    {
        fclose(file);
        return 0;
    }


    fclose(file);


    if (
        memcmp(tag, "TAG", 3) != 0
    )
    {
        return 0;
    }


    cp1251_to_utf8(
        &tag[3],
        30,
        player->track,
        sizeof(player->track)
    );


    cp1251_to_utf8(
        &tag[33],
        30,
        player->artist,
        sizeof(player->artist)
    );


    cp1251_to_utf8(
        &tag[63],
        30,
        player->album,
        sizeof(player->album)
    );


    return
        player->track[0] != '\0' ||
        player->artist[0] != '\0';
}


int metadata_load(
    const char *path,
    PlayerState *player
)
{
    player->track[0] = '\0';
    player->artist[0] = '\0';
    player->album[0] = '\0';


    /*
     * Сначала современный ID3v2.
     */
    if (read_id3v2(path, player))
        return 1;


    /*
     * Если ID3v2 нет или он не содержит
     * нужных данных — пробуем ID3v1.
     */
    if (read_id3v1(path, player))
        return 1;


    /*
     * Даже если тегов нет, оставляем
     * безопасные пустые строки.
     */

    return 0;
}
