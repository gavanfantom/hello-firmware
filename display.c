/* display.c */

#include "hello.h"
#include "display.h"
#include "font.h"
#include "i2c.h"

#define ADDRESS 0x3c

#define HORIZONTAL_FLIP 1
#define VERTICAL_FLIP 1
#define COM_ALT 1
#define COM_LR_REMAP 0
#define DISPLAY_INVERT 0
#define DEFAULT_CONTRAST 0x7f

const uint8_t display_init_seq[] = {
    0xa8, 0x3f, /* Multiplex ratio */
    0xd3, 0x00, /* Display offset */
    0x40,       /* Display start line */
    0xa0 | HORIZONTAL_FLIP,
    0xc0 | (VERTICAL_FLIP << 3),
    0xda, 0x02 | (COM_ALT << 4) | (COM_LR_REMAP << 5),
    0x81, DEFAULT_CONTRAST,
    0xa4,       /* Disable entire display on */
    0xa6 | DISPLAY_INVERT,
    0xd5, 0x80, /* Clock divide ratio */
    0x8d, 0x14, /* Charge pump setting */
    0xaf        /* Display on */
};

bool display_command(uint8_t command)
{
    i2c_transmit(ADDRESS, 0x80, &command, 1);
    while (i2c_busy())
        ;
    return i2c_result() == I2C_SUCCESS;
}

bool display_data(uint8_t *data, int len)
{
    i2c_transmit(ADDRESS, 0x40, data, len);
    while (i2c_busy())
        ;
    return i2c_result() == I2C_SUCCESS;
}

void display_setposition(uint8_t col, uint8_t row)
{
    display_command(0xb0 + (row & 0x0f));
    display_command(0x00 + (col & 0x0f));
    display_command(0x10 + ((col >> 4) & 0x0f));
}

bool display_init(void)
{
    for (int i = 0; i < ARRAY_SIZE(display_init_seq); i++)
        if (!display_command(display_init_seq[i]))
            return false;
    return true;
}

