/* write.c */

#include "hello.h"
#include "write.h"
#include "font.h"
#include "uart.h"
#include "frame.h"
#include <string.h>

int cursor_ptr;

void write_string(const char *s)
{
    for (; *s != '\0'; s++)
        font_putchar(*s);
}

void write_string_large(const char *s)
{
    for (; *s != '\0'; s++)
        font_putchar_large(*s);
}

void write_int_string(int val, char *result)
{
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
}

void write_int(int val)
{
    char result[12];
    write_int_string(val, result);
    write_string(result);
}

void uart_write_int(int val)
{
    char result[12];
    write_int_string(val, result);
    uart_write_string(result);
}

void uart_write_hex(uint32_t val)
{
    for (int i = 0; i < 8; i++) {
        uint8_t digit = val >> 28;
        val = val << 4;
        digit += (digit < 10)?'0':('A'-10);
        uart_transmit(digit);
    }
}

void uart_write_string(const char *s)
{
    for (; *s != '\0'; s++)
        uart_transmit(*s);
}

void clear_screen(void)
{
    memset(screen_buf, 0, 1024);
    cursor_ptr = 0;
}
