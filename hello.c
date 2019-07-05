/* hello.c */

#include "hello.h"
#include "uart.h"
#include "speaker.h"
#include "spi.h"
#include "spiflash.h"
#include "i2c.h"
#include "display.h"
#include "font.h"
#include "write.h"
#include <string.h>

uint8_t frame1[1024];
//uint8_t frame2[1024];
uint8_t superframe[2048];

bool usb_detect(void)
{
    return (LPC_GPIO1->DATA[DATAREG] & (1 << 2)) != 0;
}
 
void usb_connected(void)
{
    LPC_IOCON->REG[IOCON_PIO1_7] = IOCON_FUNC1; // TXD
    LPC_IOCON->REG[IOCON_PIO1_6] = IOCON_FUNC1; // RXD
    LPC_IOCON->REG[IOCON_PIO0_7] = IOCON_FUNC1; // CTS
    LPC_IOCON->REG[IOCON_PIO1_5] = IOCON_FUNC1; // RTS
}

void usb_disconnected(void)
{
    LPC_IOCON->REG[IOCON_PIO1_7] = 0;    // Turn off pull up on TXD
    LPC_IOCON->REG[IOCON_PIO1_6] = 0;    // Turn off pull up on RXD
    LPC_IOCON->REG[IOCON_PIO0_7] = 0;    // Turn off pull up on CTS
    LPC_IOCON->REG[IOCON_PIO1_5] = 0;    // Turn off pull up on RTS
}

#define BUTTON_UP     0x01
#define BUTTON_DOWN   0x02
#define BUTTON_LEFT   0x04
#define BUTTON_RIGHT  0x08
#define BUTTON_CENTRE 0x10

void buttons_init(void)
{
    LPC_IOCON->REG[IOCON_PIO3_0] = IOCON_MODE_PULLUP; // Up
    LPC_IOCON->REG[IOCON_PIO3_1] = IOCON_MODE_PULLUP; // Down
    LPC_IOCON->REG[IOCON_PIO3_2] = IOCON_MODE_PULLUP; // Left
    LPC_IOCON->REG[IOCON_PIO3_3] = IOCON_MODE_PULLUP; // Right
    LPC_IOCON->REG[IOCON_PIO3_4] = IOCON_MODE_PULLUP; // Centre
}

int button_state(void)
{
    return ~LPC_GPIO3->DATA[DATAREG] & 0x1f;
}

int main(void)
{
    LPC_SYSCTL->SYSAHBCLKCTRL   |= (1<<6);     //enable clock GPIO (sec 3.5.14)
    LPC_IOCON->REG[IOCON_PIO0_7] &= ~(0x10);    // Turn off pull up
    LPC_IOCON->REG[IOCON_PIO0_8] &= ~(0x10);    // Turn off pull up
    LPC_IOCON->REG[IOCON_PIO0_9] &= ~(0x10);    // Turn off pull up
//    LPC_GPIO->DIR              |= (1<<7) | (1<<8) | (1<<9); // outputs

    LPC_IOCON->REG[IOCON_PIO1_2] = IOCON_FUNC1 | IOCON_DIGMODE_EN;    // Turn off pull up on USBDETECT
//    LPC_IOCON->REG[IOCON_PIO0_1] = 0;    // Turn off pull up on MCU_ISP
    if (usb_detect())
        usb_connected();
    else
        usb_disconnected();

    uart_init();

    speaker_init();
    buttons_init();

#if 1
    speaker_on(1000);
    for (volatile int i = 0; i < 0x10000; i++)
        ;
    speaker_off();
#endif

#if 0
    for (volatile int i = 0; i < 0x1000000; i++)
        ;
    speaker_on(1500);
    for (volatile int i = 0; i < 0x10000; i++)
        ;
    speaker_off();
#endif

    for (volatile int i = 0; i < 0x100000; i++)
        ;

    spi_init();
    spiflash_reset();

    spiflash_address_mode3();

    i2c_init();
    display_init();
    display_setposition(0, 0);

//    LPC_IOCON->REG[IOCON_PIO0_0] &= ~(0x10);    // Turn off pull up on MCU_RESET
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

    uint8_t deviceid[3];
    deviceid[0] = 0;
    deviceid[1] = 0;
    deviceid[2] = 0;
    spiflash_read_deviceid(deviceid, sizeof(deviceid));
    spiflash_read_deviceid(deviceid, sizeof(deviceid));
    spiflash_read_deviceid(deviceid, sizeof(deviceid));

#if 0
//    write_string("123456789012345678901234567890123456789012\n");
    write_string("Superbadge has a superframe! Oooh yes\n");
    write_string("check out that scrolling action! Isn't\n");
    write_string("it amazing what we can do nowadays with\n");
    write_string("such a simple microcontroller. I really\n");

    screen_buf += 1024;
    clear_screen();

#if 0
    write_string("do think we're onto something here. So\n");
    write_string("much fun! Well, anyway, have fun, because\n");
    write_string("this screen is only so big and there's\n");
    write_string("only so much text we can fit on here. Duck\n");
#endif
    write_string("\n");
    write_int(deviceid[0]);
    write_string("\n\n");
    write_int(deviceid[1]);
    write_string("\n\n");
    write_int(deviceid[2]);
#endif
#endif
    write_string_large("Underhand");
 
    spiflash_read(0, superframe, 2048);

    int pos = 0;
#if 1
    int rate = 1;
#endif

    bool usb_present = false;

    bool written = false;

    while (1) {
        int offset = pos;
        if (offset < 0)
            offset = 0;
        if (offset > 128)
            offset = 128;
        for (int j = 0; j < 8; j++)
            memcpy(frame1 + j*128, superframe + j*256 + offset, 128);
        int buttons = button_state();
        cursor_ptr = 256*7+128;
        if (buttons < 10)
            buttons += '0';
        else
            buttons += 'A'-10;
        font_putchar(buttons);

#if 1
        if ((button_state() & BUTTON_CENTRE) && !written) {
            written = true;
            screen_buf = superframe;
            clear_screen();
            write_string_large("FLASHWRITE");
            superframe[0] = 6;
            bool ret = spiflash_write(0, superframe, 2048);
            screen_buf = superframe+1024;
            clear_screen();
            screen_buf = superframe;
            clear_screen();
            write_string_large(ret?"YES":"NO");
        }
        if ((button_state() & BUTTON_DOWN) && !written) {
            written = true;
            bool ret = spiflash_erase(0, 2048);
            screen_buf = superframe+1024;
            clear_screen();
            screen_buf = superframe;
            clear_screen();
            write_string_large(ret?"YES":"NO");
        }
#endif
        
#if 0
        if (buttons & BUTTON_LEFT)
            pos -= 1;
        if (buttons & BUTTON_RIGHT)
            pos += 1;
        if (pos < 0)
            pos = 0;
        if (pos > 128)
            pos = 128;
#else
        pos += rate;
        if (pos < 0-20) {
            pos = 0;
            rate = -rate;
        }
        if (pos > 128+20) {
            pos = 128;
            rate = -rate;
        }
#endif
        display_data(frame1, 1024);
#if 0
        display_data(frame1, 1024);
        display_data(frame2, 1024);
        screen_buf[cursor_ptr++] ^= 255;
        if (cursor_ptr >= 1024)
            cursor_ptr = 0;
#endif
        bool usb = usb_detect();
        if (!usb_present && usb) {
            beep_up();
            usb_connected();
        }
        if (usb_present && !usb) {
            beep_down();
            usb_disconnected();
        }
        usb_present = usb;
        uint8_t byte;
        if (uart_receive(&byte)) {
            uart_transmit(byte);
        }
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
