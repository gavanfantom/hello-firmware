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
#include "zmodem.h"

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
    zmodem_reinit();
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

/* Autodetect block_count if we ever want to use a different flash chip */
#define BLOCK_SIZE 4096
#define BLOCK_COUNT 2048

int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    int address = c->block_size * block + off;
    spiflash_read(address, buffer, size);
    return LFS_ERR_OK;
}

int flash_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int address = c->block_size * block + off;
    if (spiflash_write(address, (void *)buffer, size))
        return LFS_ERR_OK;
    return LFS_ERR_IO;
}

int flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    int address = c->block_size * block;
    if (spiflash_erase_sector(address))
        return LFS_ERR_OK;
    return LFS_ERR_IO;
}

int flash_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK;
}

lfs_t lfs;
lfs_file_t file;

uint8_t read_buffer[16];
uint8_t prog_buffer[16];
uint8_t lookahead_buffer[16] __attribute__ ((aligned (8)));

const struct lfs_config cfg = {
    .read = flash_read,
    .prog = flash_write,
    .erase = flash_erase,
    .sync = flash_sync,

    .read_size = 1,
    .prog_size = 1,
    .block_size = BLOCK_SIZE,
    .block_count = BLOCK_COUNT,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 1000,
    .read_buffer = read_buffer,
    .prog_buffer = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};

void HardFault_Handler( void ) __attribute__( ( naked ) );
void HardFault_Handler(void)
{
    __asm volatile
    (
        " mrs r0, msp                                               \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    uart_write_string("FAULT\r\n");
    uart_write_hex(r0);
    uart_write_string("\r\n");
    uart_write_hex(r1);
    uart_write_string("\r\n");
    uart_write_hex(r2);
    uart_write_string("\r\n");
    uart_write_hex(r3);
    uart_write_string("\r\n");
    uart_write_hex(r12);
    uart_write_string("\r\n");
    uart_write_hex(lr);
    uart_write_string("\r\n");
    uart_write_hex(pc);
    uart_write_string("\r\n");
    uart_write_hex(psr);
    uart_write_string("\r\n");

    for( ;; );
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

    uart_init();
    zmodem_init(&lfs, &file);

    speaker_init();
    buttons_init();

#if 1
    speaker_on(1000);
    for (volatile int i = 0; i < 0x10000; i++)
        ;
    speaker_off();
#endif

//    for (int j = 0; j < 10; j++)
    for (volatile int i = 0; i < 0x100000; i++)
        ;

#if 1
    speaker_on(1500);
    for (volatile int i = 0; i < 0x10000; i++)
        ;
    speaker_off();
#endif

//    uart_write_string("Hello\r\n");

#if 1
    for (volatile int i = 0; i < 0x10000; i++)
        ;
    speaker_on(2000);
    for (volatile int i = 0; i < 0x10000; i++)
        ;
    speaker_off();
#endif

    spi_init();
    spiflash_reset();

    spiflash_address_mode3();

    int err = lfs_mount(&lfs, &cfg);
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    i2c_init();
    display_init();
    display_setposition(0, 0);

    display_command(0x20);
    display_command(0);

    // Let's draw a superframe. frame1 will be the top half and
    // frame2 will be the bottom half. Then we'll stitch them together
    // into a superframe and select the appropriate part for each real
    // frame.

    screen_buf = superframe;
    clear_screen();

    cursor_ptr = 0;
    write_string("Hello");

#if 1
    int ret = lfs_file_open(&lfs, &file, "name", LFS_O_RDONLY);
    if (!ret) {
        lfs_file_read(&lfs, &file, superframe, 2048);
        lfs_file_close(&lfs, &file);
    }
#endif

    /* These will be useful until a better way to
     * remove a file exists
     */
//    lfs_remove(&lfs, "name");
//    lfs_unmount(&lfs);

//    speaker_on(2000);
//    spiflash_erase_chip();
//    speaker_off();

    int pos = 0;
    int rate = 1;

    bool usb_present = false;
    int norxframes = 0;

    while (1) {
        int offset = pos;
        if (offset < 0)
            offset = 0;
        if (offset > 128)
            offset = 128;
        if (zmodem_active()) {
            screen_buf = frame1;
            clear_screen();
            write_int(zmodem_debug());
            int progress = zmodem_progress();
            frame1[128*3 + 13] = 255;
            frame1[128*4 + 13] = 255;
            frame1[128*3 + 115] = 255;
            frame1[128*4 + 115] = 255;
            if (progress < 0) {
                for (int i = 0; i < 101; i++) {
                    frame1[128*3 + 14 + i] = (i & 1)?0xAB:0x55;
                    frame1[128*4 + 14 + i] = (i & 1)?0xAA:0xD5;
                }
            } else {
                for (int i = 0; i < 101; i++) {
                    frame1[128*3 + 14 + i] = (i < progress)?255:1;
                    frame1[128*4 + 14 + i] = (i < progress)?255:128;
                }
            }
        } else {
            for (int j = 0; j < 8; j++)
                memcpy(frame1 + j*128, superframe + j*256 + offset, 128);
#if 0
            screen_buf = frame1;
            cursor_ptr = 0;
            write_int(norxframes);
#endif
        }

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
#if 0
        if (uart_receive(&byte)) {
            uart_transmit(byte);
        }
#else
        while (uart_receive(&byte)) {
            norxframes = 0;
            zrx_byte(byte);
        }
        /* XXX Once we have a timer, we can improve on this */
        if (++norxframes > 1500) {
            norxframes = 0;
            zmodem_timeout();
        }
#endif
    }

    return 0;
}
