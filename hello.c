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
#include "lfs.h"
#include "fs.h"
#include "zmodem.h"
#include "timer.h"
#include "cli.h"
#include "lock.h"
#include "button.h"
#include "file.h"
#include "frame.h"

#define SYSTIME_FREQUENCY 1000

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
    uart_init();
}

void usb_disconnected(void)
{
    LPC_IOCON->REG[IOCON_PIO1_7] = 0;    // Turn off pull up on TXD
    LPC_IOCON->REG[IOCON_PIO1_6] = 0;    // Turn off pull up on RXD
    LPC_IOCON->REG[IOCON_PIO0_7] = 0;    // Turn off pull up on CTS
    LPC_IOCON->REG[IOCON_PIO1_5] = 0;    // Turn off pull up on RTS
    uart_pause();
    cli_reinit();
    zmodem_reinit();
    unlock_file();
}

void set_interrupt_priorities(void)
{
    NVIC_SetPriority(UART0_IRQn, 0);
    NVIC_SetPriority(TIMEOUT_IRQN, 0);
    NVIC_SetPriority(I2C0_IRQn, 1);
    NVIC_SetPriority(FRAME_IRQN, 1);
}

int main(void)
{
    LPC_SYSCTL->SYSAHBCLKCTRL   |= (1<<6);     //enable clock GPIO (sec 3.5.14)
    LPC_IOCON->REG[IOCON_PIO0_7] &= ~(0x10);    // Turn off pull up
    LPC_IOCON->REG[IOCON_PIO0_8] &= ~(0x10);    // Turn off pull up
    LPC_IOCON->REG[IOCON_PIO0_9] &= ~(0x10);    // Turn off pull up

    LPC_IOCON->REG[IOCON_PIO1_2] = IOCON_FUNC1 | IOCON_DIGMODE_EN;    // Turn off pull up on USBDETECT
//    LPC_IOCON->REG[IOCON_PIO0_1] = 0;    // Turn off pull up on MCU_ISP

    if (usb_detect())
        usb_connected();
    else
        usb_disconnected();

    set_interrupt_priorities();

    systime_timer_init();
    systime_timer_on(SYSTIME_FREQUENCY);

    lock_init();
    uart_init();
    cli_init();

    speaker_init();
    button_init();

    beep(500, 20);
    delay(500);
    beep(750, 20);
    delay(20);
    beep(1000, 20);

    fs_init();

    frame_init();

    /* This may be useful until a better way to
     * erase the entire filesystem exists
     */

//    speaker_on(1000);
//    spiflash_erase_chip();
//    speaker_off();

    bool safe = (button_state() & BUTTON_CENTRE)?true:false;
    file_init(safe);

    bool usb_present = false;

    while (1) {
        check_file_lock();
        frame_update();

        int pressed = button_event();
        if (pressed & BUTTON_LEFT)
            prev_file();
        if (pressed & BUTTON_RIGHT)
            next_file();
        if (pressed & BUTTON_UP)
            contrast_up();
        if (pressed & BUTTON_DOWN)
            contrast_down();

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
    }

    return 0;
}
