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


/* Some displays seem to have an unusual behaviour
 * whereby writing to the display wraps around to
 * the top after only 7 out of 8 of the pages.
 * It should be possible to reset this behaviour by
 * writing to addressing mode registers, but it
 * apparently isn't.
 * We can work around this by always starting the
 * write at the beginning of the last page, and then
 * shifting the display start so that the last page
 * is the first to be displayed.
 */

#define CRAZY_DISPLAY_WORKAROUND

#ifdef CRAZY_DISPLAY_WORKAROUND
# define DISPLAY_START_LINE 0x38
# define FIRST_PAGE 0x07
#else
# define DISPLAY_START_LINE 0x00
# define FIRST_PAGE 0x00
#endif

const uint8_t display_init_seq[] = {
    0xa8, 0x3f, /* Multiplex ratio */
    0xd3, 0x00, /* Display offset */
    0x40 | DISPLAY_START_LINE,  /* Display start line */
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
    while (!i2c_transmit(ADDRESS, 0x80, &command, 1, NULL))
        ;
    while (i2c_busy())
        ;
    return i2c_result() == I2C_SUCCESS;
}

static int display_state;
static uint8_t display_contrast;
static uint8_t byte;

#include "write.h"

void display_callback(void)
{
    i2c_fn fn = display_callback;
    switch (display_state++) {
    case 0:
        byte = 0x81;
        break;
    case 1:
        byte = display_contrast;
        break;
    case 2:
        byte = 0xb0 | FIRST_PAGE;
        break;
    case 3:
        byte = 0x00;
        break;
    case 4:
        byte = 0x10;
        fn = NULL;
        break;
    default:
        byte = 0;
        fn = NULL;
        break;
    }
    i2c_transmit(ADDRESS, 0x80, &byte, 1, fn);
}

bool display_start(uint8_t *data, int len, int contrast)
{
    if (contrast >= 0) {
        display_contrast = contrast;
        display_state = 0;
    } else
        display_state = 2;
    return i2c_transmit(ADDRESS, 0x40, data, len, display_callback);
}

bool display_data(uint8_t *data, int len)
{
    i2c_transmit(ADDRESS, 0x40, data, len, NULL);
    while (i2c_busy())
        ;
    return i2c_result() == I2C_SUCCESS;
}

void display_setposition(uint8_t col, uint8_t row)
{
    display_command(0xb0 + ((row + FIRST_PAGE) & 0x0f));
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

