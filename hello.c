/* hello.c */

#include <string.h>
#include "chip.h"

//#define I2C_FASTMODEPLUS
#define I2C_EXTRA

#ifdef I2C_EXTRA
# define I2CPINCONFIG (IOCON_FUNC1 | IOCON_FASTI2C_EN)
# define I2C_SPEED 2600000
# define SCL_LOWDUTY 75
#elif defined(I2C_FASTMODEPLUS)
# define I2CPINCONFIG (IOCON_FUNC1 | IOCON_FASTI2C_EN)
# define I2C_SPEED 1000000
#elif defined(I2C_FAST)
# define I2CPINCONFIG (IOCON_FUNC1 | IOCON_STDI2C_EN)
# define I2C_SPEED 400000
#else
# define I2CPINCONFIG (IOCON_FUNC1 | IOCON_STDI2C_EN)
# define I2C_SPEED 100000
#endif

#define I2C_PCLK 48000000
#define SCL_PERIOD (I2C_PCLK / I2C_SPEED)
#ifndef SCL_LOWDUTY
# define SCL_LOWDUTY 50
#endif
#define SCL_HIGHDUTY (100 - (SCL_LOWDUTY))
#define SCLL_VALUE (SCL_LOWDUTY  * SCL_PERIOD / 100)
#define SCLH_VALUE (SCL_HIGHDUTY * SCL_PERIOD / 100)

#define DATAREG (0x3ffc >> 2)
#define SET_LED(x) LPC_GPIO->DATA[DATAREG] = (LPC_GPIO->DATA[DATAREG] & ~(7<<7)) | (x << 7)

#define WHITE   0
#define CYAN    1
#define MAGENTA 2
#define BLUE    3
#define YELLOW  4
#define GREEN   5
#define RED     6
#define BLACK   7

void i2c_init(void)
{
    LPC_IOCON->REG[IOCON_PIO0_4] = I2CPINCONFIG;
    LPC_IOCON->REG[IOCON_PIO0_5] = I2CPINCONFIG;
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_I2C);
    Chip_SYSCTL_DeassertPeriphReset(RESET_I2C0);
    LPC_I2C->CONSET = I2C_I2CONSET_I2EN;
    LPC_I2C->SCLH = SCLH_VALUE;
    LPC_I2C->SCLL = SCLL_VALUE;
    NVIC_EnableIRQ(I2C0_IRQn);
}


volatile struct {
    uint8_t *data;
    int len;
    int header;
    int offset;
    int address;
    volatile int result;
} i2c_state;

#define I2C_SUCCESS             0
#define I2C_BUSY                1
#define I2C_FAIL_BUS_ERROR      2
#define I2C_FAIL_NACK           3
#define I2C_FAIL_ARBITRATION    4
#define I2C_FAIL_NOTIMPLEMENTED 5

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

bool i2c_busy(void)
{
    return i2c_state.result == I2C_BUSY;
}

void i2c_transmit(uint8_t address, uint8_t header, uint8_t *data, int len)
{
    while (i2c_busy())
        ;

    i2c_state.address = address << 1;
    i2c_state.header = header;
    i2c_state.data = data;
    i2c_state.len = len;
    i2c_state.offset = -1;
    i2c_state.result = I2C_BUSY;
    LPC_I2C->CONSET = I2C_I2CONSET_STA;
}

int i2c_result(void)
{
    return i2c_state.result;
}

void I2C0_IRQHandler(void)
{
    int state = LPC_I2C->STAT;
    switch (state) {
    case 0x00: // Bus error
        LPC_I2C->CONSET = I2C_I2CONSET_STO | I2C_I2CONSET_AA;
        LPC_I2C->CONCLR = I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC;
        i2c_state.result = I2C_FAIL_BUS_ERROR;
        break;
    case 0x08: // START has been transmitted
    case 0x10: // Repeated START has been transmitted
        LPC_I2C->DAT = i2c_state.address;
        LPC_I2C->CONSET = I2C_I2CONSET_AA;
        LPC_I2C->CONCLR = I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC;
        break;
    case 0x18: // SLA+W has been transmitted, ACK has been received
    case 0x28: // Data has been transmitted, ACK has been received
        if (i2c_state.offset < 0) {
            LPC_I2C->DAT = i2c_state.header;
            i2c_state.offset++;
            LPC_I2C->CONSET = I2C_I2CONSET_AA;
            LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
        } else if (i2c_state.offset < i2c_state.len) {
            LPC_I2C->DAT = i2c_state.data[i2c_state.offset++];
            LPC_I2C->CONSET = I2C_I2CONSET_AA;
            LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
        } else {
            LPC_I2C->CONSET = I2C_I2CONSET_STO | I2C_I2CONSET_AA;
            LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
            i2c_state.result = I2C_SUCCESS;
        }
        break;
    case 0x20: // SLA+W has been transmitted, NACK has been received
    case 0x30: // Data has been transmitted, NACK has been received
        LPC_I2C->CONSET = I2C_I2CONSET_STO | I2C_I2CONSET_AA;
        LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
        i2c_state.result = I2C_FAIL_NACK;
        break;
    case 0x38: // Arbitration lost
        LPC_I2C->CONSET = I2C_I2CONSET_STO | I2C_I2CONSET_AA;
        LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
        i2c_state.result = I2C_FAIL_ARBITRATION;
        break;
    default:
        LPC_I2C->CONSET = I2C_I2CONSET_STO | I2C_I2CONSET_AA;
        LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
        i2c_state.result = I2C_FAIL_NOTIMPLEMENTED;
        break;
    }
  //  LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
}

#define ADDRESS 0x3c

#define HORIZONTAL_FLIP 0
#define VERTICAL_FLIP 0
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

uint8_t frame1[1024];
//uint8_t frame2[1024];
uint8_t superframe[2048];

static const uint8_t font[] = {
	0x00, 0x00, 0x00, 0x00, 0x00,// (space)
	0x00, 0x00, 0x5F, 0x00, 0x00,// !
	0x00, 0x07, 0x00, 0x07, 0x00,// "
	0x14, 0x7F, 0x14, 0x7F, 0x14,// #
	0x24, 0x2A, 0x7F, 0x2A, 0x12,// $
	0x23, 0x13, 0x08, 0x64, 0x62,// %
	0x36, 0x49, 0x55, 0x22, 0x50,// &
	0x00, 0x05, 0x03, 0x00, 0x00,// '
	0x00, 0x1C, 0x22, 0x41, 0x00,// (
	0x00, 0x41, 0x22, 0x1C, 0x00,// )
	0x08, 0x2A, 0x1C, 0x2A, 0x08,// *
	0x08, 0x08, 0x3E, 0x08, 0x08,// +
	0x00, 0x50, 0x30, 0x00, 0x00,// ,
	0x08, 0x08, 0x08, 0x08, 0x08,// -
	0x00, 0x60, 0x60, 0x00, 0x00,// .
	0x20, 0x10, 0x08, 0x04, 0x02,// /
	0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
	0x00, 0x42, 0x7F, 0x40, 0x00,// 1
	0x42, 0x61, 0x51, 0x49, 0x46,// 2
	0x21, 0x41, 0x45, 0x4B, 0x31,// 3
	0x18, 0x14, 0x12, 0x7F, 0x10,// 4
	0x27, 0x45, 0x45, 0x45, 0x39,// 5
	0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
	0x01, 0x71, 0x09, 0x05, 0x03,// 7
	0x36, 0x49, 0x49, 0x49, 0x36,// 8
	0x06, 0x49, 0x49, 0x29, 0x1E,// 9
	0x00, 0x36, 0x36, 0x00, 0x00,// :
	0x00, 0x56, 0x36, 0x00, 0x00,// ;
	0x00, 0x08, 0x14, 0x22, 0x41,// <
	0x14, 0x14, 0x14, 0x14, 0x14,// =
	0x41, 0x22, 0x14, 0x08, 0x00,// >
	0x02, 0x01, 0x51, 0x09, 0x06,// ?
	0x32, 0x49, 0x79, 0x41, 0x3E,// @
	0x7E, 0x11, 0x11, 0x11, 0x7E,// A
	0x7F, 0x49, 0x49, 0x49, 0x36,// B
	0x3E, 0x41, 0x41, 0x41, 0x22,// C
	0x7F, 0x41, 0x41, 0x22, 0x1C,// D
	0x7F, 0x49, 0x49, 0x49, 0x41,// E
	0x7F, 0x09, 0x09, 0x01, 0x01,// F
	0x3E, 0x41, 0x41, 0x51, 0x32,// G
	0x7F, 0x08, 0x08, 0x08, 0x7F,// H
	0x00, 0x41, 0x7F, 0x41, 0x00,// I
	0x20, 0x40, 0x41, 0x3F, 0x01,// J
	0x7F, 0x08, 0x14, 0x22, 0x41,// K
	0x7F, 0x40, 0x40, 0x40, 0x40,// L
	0x7F, 0x02, 0x04, 0x02, 0x7F,// M
	0x7F, 0x04, 0x08, 0x10, 0x7F,// N
	0x3E, 0x41, 0x41, 0x41, 0x3E,// O
	0x7F, 0x09, 0x09, 0x09, 0x06,// P
	0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
	0x7F, 0x09, 0x19, 0x29, 0x46,// R
	0x46, 0x49, 0x49, 0x49, 0x31,// S
	0x01, 0x01, 0x7F, 0x01, 0x01,// T
	0x3F, 0x40, 0x40, 0x40, 0x3F,// U
	0x1F, 0x20, 0x40, 0x20, 0x1F,// V
	0x7F, 0x20, 0x18, 0x20, 0x7F,// W
	0x63, 0x14, 0x08, 0x14, 0x63,// X
	0x03, 0x04, 0x78, 0x04, 0x03,// Y
	0x61, 0x51, 0x49, 0x45, 0x43,// Z
	0x00, 0x00, 0x7F, 0x41, 0x41,// [
	0x02, 0x04, 0x08, 0x10, 0x20,// "\"
	0x41, 0x41, 0x7F, 0x00, 0x00,// ]
	0x04, 0x02, 0x01, 0x02, 0x04,// ^
	0x40, 0x40, 0x40, 0x40, 0x40,// _
	0x00, 0x01, 0x02, 0x04, 0x00,// `
	0x20, 0x54, 0x54, 0x54, 0x78,// a
	0x7F, 0x48, 0x44, 0x44, 0x38,// b
	0x38, 0x44, 0x44, 0x44, 0x20,// c
	0x38, 0x44, 0x44, 0x48, 0x7F,// d
	0x38, 0x54, 0x54, 0x54, 0x18,// e
	0x08, 0x7E, 0x09, 0x01, 0x02,// f
	0x08, 0x14, 0x54, 0x54, 0x3C,// g
	0x7F, 0x08, 0x04, 0x04, 0x78,// h
	0x00, 0x44, 0x7D, 0x40, 0x00,// i
	0x20, 0x40, 0x44, 0x3D, 0x00,// j
	0x00, 0x7F, 0x10, 0x28, 0x44,// k
	0x00, 0x41, 0x7F, 0x40, 0x00,// l
	0x7C, 0x04, 0x18, 0x04, 0x78,// m
	0x7C, 0x08, 0x04, 0x04, 0x78,// n
	0x38, 0x44, 0x44, 0x44, 0x38,// o
	0x7C, 0x14, 0x14, 0x14, 0x08,// p
	0x08, 0x14, 0x14, 0x18, 0x7C,// q
	0x7C, 0x08, 0x04, 0x04, 0x08,// r
	0x48, 0x54, 0x54, 0x54, 0x20,// s
	0x04, 0x3F, 0x44, 0x40, 0x20,// t
	0x3C, 0x40, 0x40, 0x20, 0x7C,// u
	0x1C, 0x20, 0x40, 0x20, 0x1C,// v
	0x3C, 0x40, 0x30, 0x40, 0x3C,// w
	0x44, 0x28, 0x10, 0x28, 0x44,// x
	0x0C, 0x50, 0x50, 0x50, 0x3C,// y
	0x44, 0x64, 0x54, 0x4C, 0x44,// z
	0x00, 0x08, 0x36, 0x41, 0x00,// {
	0x00, 0x00, 0x7F, 0x00, 0x00,// |
	0x00, 0x41, 0x36, 0x08, 0x00,// }
	0x08, 0x08, 0x2A, 0x1C, 0x08,// ->
	0x08, 0x1C, 0x2A, 0x08, 0x08 // <-
};

uint8_t *screen_buf;
int cursor_ptr;

void font_putchar(char ch)
{
    uint8_t c = ch - 32;

    if (ch == '\n') {
        cursor_ptr = (cursor_ptr & ~0x7f) + 0x80;
        if (cursor_ptr >= 1024-5)
            cursor_ptr = 0;
        return;
    }

    if (c > (127-32))
        c = 0;

    memcpy(screen_buf + cursor_ptr, font+5*c, 5);
    screen_buf[cursor_ptr+5] = 0;

    cursor_ptr += 6;
    if (cursor_ptr >= 1024-5)
        cursor_ptr = 0;
}

void write_string(char *s)
{
    for (; *s != '\0'; s++)
        font_putchar(*s);
}

void write_int(int val)
{
    char result[12];
    bool minus = (val < 0);

    if (val >= 0)
        val = -val;

    int i = 0;

    while (val) {
        result[i++] = '0' - val % 10;
        val = val / 10;
    }
    if (i == 0)
        result[i++] = '0';

    if (minus)
        result[i++] = '-';
    result[i] = '\0';
    for (int j = 0; j < i/2; j++) {
        char tmp = result[j];
        result[j] = result[i-j-1];
        result[i-j-1] = tmp;
    }
    write_string(result);
}

void clear_screen(void)
{
    memset(screen_buf, 0, 1024);
    cursor_ptr = 0;
}

int main(void)
{
    LPC_SYSCTL->SYSAHBCLKCTRL   |= (1<<6);     //enable clock GPIO (sec 3.5.14)
    LPC_IOCON->REG[IOCON_PIO0_7] &= ~(0x10);    // Turn off pull up
    LPC_IOCON->REG[IOCON_PIO0_8] &= ~(0x10);    // Turn off pull up
    LPC_IOCON->REG[IOCON_PIO0_9] &= ~(0x10);    // Turn off pull up
    LPC_GPIO->DIR              |= (1<<7) | (1<<8) | (1<<9); // outputs

    i2c_init();
    display_init();
    display_setposition(0, 0);

    display_command(0x20);
    display_command(0);
#if 0
    screen_buf = frame1;
    clear_screen();
    write_string("Hello\n");
    write_string("This is a mini badge\n");
    write_string("Woohoo!\n");
    write_int(LPC_I2C->SCLL);
    write_string(" / ");
    write_int(LPC_I2C->SCLH);
    write_string("\n");
    screen_buf = frame2;
    clear_screen();
//    write_string("\n\n\nThis is frame 2\n\nAll hail the hypnobadge\n");
#endif

    // Let's draw a superframe. frame1 will be the top half and
    // frame2 will be the bottom half. Then we'll stitch them together
    // into a superframe and select the appropriate part for each real
    // frame.

#if 1
    screen_buf = superframe;
    clear_screen();

//    write_string("123456789012345678901234567890123456789012\n");
    write_string("Superbadge has a superframe! Oooh yes\n");
    write_string("check out that scrolling action! Isn't\n");
    write_string("it amazing what we can do nowadays with\n");
    write_string("such a simple microcontroller. I really\n");

    screen_buf += 1024;
    clear_screen();

    write_string("do think we're onto something here. So\n");
    write_string("much fun! Well, anyway, have fun, because\n");
    write_string("this screen is only so big and there's\n");
    write_string("only so much text we can fit on here. Duck\n");
#endif
 
    int pos = 0;
    int rate = 1;

    while (1) {
        int offset = pos;
        if (offset < 0)
            offset = 0;
        if (offset > 128)
            offset = 128;
        for (int j = 0; j < 8; j++)
            memcpy(frame1 + j*128, superframe + j*256 + offset, 128);
        pos += rate;
        if (pos < 0-20) {
            pos = 0;
            rate = -rate;
        }
        if (pos > 128+20) {
            pos = 128;
            rate = -rate;
        }
        display_data(frame1, 1024);
#if 0
        display_data(frame1, 1024);
        display_data(frame2, 1024);
        screen_buf[cursor_ptr++] ^= 255;
        if (cursor_ptr >= 1024)
            cursor_ptr = 0;
#endif
    }

    volatile unsigned int i = 0;
    unsigned int v = 0;

    while(1) {
	LPC_GPIO->DATA[DATAREG] = (LPC_GPIO->DATA[DATAREG] & ~(7<<7)) | (v << 7);
	for(i=0; i<0xFFFFF; ++i);               //arbitrary delay
        v++;
        if (v > 7)
	    v = 0;
    }

    return 0;
}
