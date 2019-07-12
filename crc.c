/* crc.c */

#include <stdint.h>
#include "crc.h"

static uint16_t crc16_table[] =
{
    0x0000,
    0x1021,
    0x2042,
    0x3063,
    0x4084,
    0x50a5,
    0x60c6,
    0x70e7,
    0x8108,
    0x9129,
    0xa14a,
    0xb16b,
    0xc18c,
    0xd1ad,
    0xe1ce,
    0xf1ef
};

static uint32_t crc32_table[] =
{
    0x00000000,
    0x1DB71064,
    0x3B6E20C8,
    0x26D930AC,
    0x76DC4190,
    0x6B6B51F4,
    0x4DB26158,
    0x5005713C,
    0xEDB88320,
    0xF00F9344,
    0xD6D6A3E8,
    0xCB61B38C,
    0x9B64C2B0,
    0x86D3D2D4,
    0xA00AE278,
    0xBDBDF21C
};

uint16_t crc16(const void *data, int length, uint16_t crc)
{
    const uint8_t *buffer = data;
    while (length--)
    {
        crc = crc16_table[(crc >> 12) ^ (*buffer >> 4)]   ^ (crc << 4);
        crc = crc16_table[(crc >> 12) ^ (*buffer & 0x0f)] ^ (crc << 4);
        buffer++;
    }
    return crc;
}

uint32_t crc32(const void *data, int length, uint32_t crc)
{
    const uint8_t *buffer = data;
    crc = ~crc;
    while (length--)
    {
        crc = crc32_table[(crc ^  *buffer      ) & 0x0f] ^ (crc >> 4);
        crc = crc32_table[(crc ^ (*buffer >> 4)) & 0x0f] ^ (crc >> 4);
        buffer++;
    }
    return ~crc;
}
