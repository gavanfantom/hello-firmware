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
#include "timer.h"

uint8_t frame1[1024];
uint8_t frame2[1024];
uint8_t *display_buf;
bool draw;

bool file_locked;
bool file_locked_request;

#define CMD_BUFFER_SIZE ZMODEM_BUFFER_SIZE

#define STATE_CLI 0
#define STATE_ZMODEM 1

int uart_state = STATE_CLI;
char *cmd_buffer;
int cmd_ptr = 0;
int arg_ptr = 0;

void unlock_file(void);

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
    cmd_ptr = 0;
    unlock_file();
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
bool file_open;

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

void zmodem_draw_progress(uint8_t *frame)
{
    screen_buf = frame;
    clear_screen();
//    write_int(zmodem_debug());
    int progress = zmodem_progress();
    frame[128*3 + 13] = 255;
    frame[128*4 + 13] = 255;
    frame[128*3 + 115] = 255;
    frame[128*4 + 115] = 255;
    if (progress < 0) {
        for (int i = 0; i < 101; i++) {
            frame[128*3 + 14 + i] = (i & 1)?0xAB:0x55;
            frame[128*4 + 14 + i] = (i & 1)?0xAA:0xD5;
        }
    } else {
        for (int i = 0; i < 101; i++) {
            frame[128*3 + 14 + i] = (i < progress)?255:1;
            frame[128*4 + 14 + i] = (i < progress)?255:128;
        }
    }
}

void draw_cli(uint8_t *frame)
{
    screen_buf = frame;
    clear_screen();
    uint32_t topbits = 0xff;
    uint32_t bottombits = 0xff000000;
    for (int i = 0; i < 32; i++) {
        frame[128*0 + 8 + i] = (topbits >> 0) & 0xff;
        frame[128*1 + 8 + i] = (topbits >> 8) & 0xff;
        frame[128*2 + 8 + i] = (topbits >> 16) & 0xff;
        frame[128*3 + 8 + i] = (topbits >> 24) & 0xff;
        frame[128*4 + 8 + i] = (bottombits >> 0) & 0xff;
        frame[128*5 + 8 + i] = (bottombits >> 8) & 0xff;
        frame[128*6 + 8 + i] = (bottombits >> 16) & 0xff;
        frame[128*7 + 8 + i] = (bottombits >> 24) & 0xff;
        topbits <<= 1;
        bottombits >>= 1;
    }
}

void draw_error(uint8_t *frame)
{
    screen_buf = frame;
    clear_screen();
    for (int i = 0; i < 8; i++) {
        frame[128*0 + 60 + i] = 0x00;
        frame[128*1 + 60 + i] = 0xff;
        frame[128*2 + 60 + i] = 0xff;
        frame[128*3 + 60 + i] = 0xff;
        frame[128*4 + 60 + i] = 0xff;
        frame[128*5 + 60 + i] = 0x00;
        frame[128*6 + 60 + i] = 0xff;
        frame[128*7 + 60 + i] = 0x00;
    }
}

struct {
    int pos;
    int rate;
    int width;
    int action;
    int pause;
    int blanks;
} imagedata;

#define ACTION_LOOP 0
#define ACTION_REVERSE 1

void image_load(const char *filename)
{
    imagedata.pos = 0;
    imagedata.rate = 1;
    imagedata.width = 256;
    imagedata.action = ACTION_REVERSE;
    imagedata.pause = 20;
    imagedata.blanks = 0;
    int ret = lfs_file_open(&lfs, &file, filename, LFS_O_RDONLY);
    if (ret)
        return;
    ret = lfs_file_size(&lfs, &file);
    if (ret < 0) {
        lfs_file_close(&lfs, &file);
        return;
    }
    imagedata.width = ret / 8;
    file_open = true;
}

void start_display(void)
{
    image_load("name");
}

void draw_image(uint8_t *frame)
{
    int offset = imagedata.pos;
    if (offset < 0)
        offset = 0;
    if (offset > imagedata.width - 128)
        offset = imagedata.width - 128;
    for (int j = 0; j < 8; j++) {
        int ret;
        ret = lfs_file_seek(&lfs, &file, offset + j*imagedata.width, LFS_SEEK_SET);
        if (ret < 0) {
            lfs_file_close(&lfs, &file);
            file_open = 0;
            return;
        }
        ret = lfs_file_read(&lfs, &file, frame + j*128, 128);
        if (ret < 0) {
            lfs_file_close(&lfs, &file);
            file_open = 0;
            return;
        }
    }
#if 0
    cursor_ptr = 0;
    write_int(norxframes);
#endif

    imagedata.pos += imagedata.rate;
    if (imagedata.pos < 0-imagedata.pause) {
        imagedata.pos = 0;
        imagedata.rate = -imagedata.rate;
    }
    if (imagedata.pos > imagedata.width - 128 +imagedata.pause) {
        imagedata.pos = imagedata.width - 128;
        imagedata.rate = -imagedata.rate;
    }
}

void FRAME_HANDLER(void)
{
    FRAME_BASE->IR = 15;
    display_start(display_buf, 1024);
    draw = true;
}

void check_file_lock(void)
{
    if (file_locked_request) {
        if (file_open) {
            lfs_file_close(&lfs, &file);
            file_open = false;
        }
        file_locked = true;
        file_locked_request = false;
        uart_resume();
    }
    if (!file_locked && !file_open) {
        start_display();
    }
}

void unlock_file(void)
{
    file_locked = false;
}

void lock_file(void)
{
    file_locked_request = true;
    uart_pause();
}

void cmd_rz(void)
{
    lock_file();
    zmodem_setactive();
    uart_state = STATE_ZMODEM;
}

void cmd_ls(void)
{
    lfs_dir_t dir;
    int err = lfs_dir_open(&lfs, &dir, "/");
    if (err) {
        uart_write_string("Error opening directory\r\n");
        return;
    }
    struct lfs_info info;
    while (true) {
        int res  = lfs_dir_read(&lfs, &dir, &info);
        if (res < 0) {
            uart_write_string("Error reading directory\r\n");
            lfs_dir_close(&lfs, &dir);
        }
        if (res == 0)
            break;
        switch (info.type) {
        case LFS_TYPE_REG:
            uart_write_string("f ");
            break;
        case LFS_TYPE_DIR:
            uart_write_string("d ");
            break;
        default:
            uart_write_string("? ");
            break;
        }
        int len = 1;
        for (int size = info.size; size >= 10; size /= 10)
            len++;
        for (int i = 10-len; i; i--)
            uart_write_string(" ");
        uart_write_int(info.size);
        uart_write_string(" ");
        uart_write_string(info.name);
        uart_write_string("\r\n");
    }
    lfs_dir_close(&lfs, &dir);
}

const char *lfs_error(int error)
{
    switch (error) {
    case LFS_ERR_IO:          return "I/O error";
    case LFS_ERR_CORRUPT:     return "Corrupted";
    case LFS_ERR_NOENT:       return "File not found";
    case LFS_ERR_EXIST:       return "Entry already exists";
    case LFS_ERR_NOTDIR:      return "Not a directory";
    case LFS_ERR_ISDIR:       return "Is a directory";
    case LFS_ERR_NOTEMPTY:    return "Directory not empty";
    case LFS_ERR_BADF:        return "Bad file number";
    case LFS_ERR_FBIG:        return "File too big";
    case LFS_ERR_INVAL:       return "Invalid argument";
    case LFS_ERR_NOSPC:       return "No space left on device";
    case LFS_ERR_NOMEM:       return "Out of memory";
    case LFS_ERR_NOATTR:      return "No attribute";
    case LFS_ERR_NAMETOOLONG: return "File name too long";
    }
    return "Unknown error";
}

void cmd_rm(void)
{
    if (arg_ptr == 0) {
        uart_write_string("Usage: rm <filename>\r\n");
        return ;
    }

    char *filename = cmd_buffer + arg_ptr;

    int err = lfs_remove(&lfs, filename);
    if (err) {
        uart_write_string(lfs_error(err));
        uart_write_string("\r\n");
        return;
    }
}

void cmd_free(void)
{
    lfs_ssize_t res = lfs_fs_size(&lfs);
    uart_write_int(res);
    uart_write_string("/");
    uart_write_int(BLOCK_COUNT);
    uart_write_string(" blocks used (");
    uart_write_int(100*res/BLOCK_COUNT);
    uart_write_string("%)\r\n");
}

typedef struct {
    const char *command;
    void (*fn)(void);
} command;

void cmd_help(void);

static command cmd_table[] = {
    {"help", &cmd_help},
    {"rz", &cmd_rz},
    {"ls", &cmd_ls},
    {"rm", &cmd_rm},
    {"free", &cmd_free},
};

void process_command(void)
{
    cmd_buffer[cmd_ptr] = '\0';

    cmd_ptr = 0;

    char *cmd = cmd_buffer + strspn(cmd_buffer, " ");
    char *delim = strchr(cmd, ' ');
    if (delim) {
        char *arg = delim + strspn(delim, " ");
        arg_ptr = arg - cmd_buffer;
        *delim = '\0';
    } else {
        arg_ptr = 0;
    }

    for (int i = 0; i < ARRAY_SIZE(cmd_table); i++) {
        if (strcmp(cmd, cmd_table[i].command) == 0) {
            cmd_table[i].fn();
            return;
        }
    }
    uart_write_string("Unrecognised command\r\n");
}

void cmd_help(void)
{
    for (int i = 0; i < ARRAY_SIZE(cmd_table); i++) {
        uart_write_string(cmd_table[i].command);
        uart_write_string("\r\n");
    }
}

void process_byte(uint8_t byte)
{
    switch (uart_state) {
    case STATE_CLI:
        switch (byte) {
        case 0:
            break;
        case 3: // Ctrl-C
            cmd_ptr = 0;
            uart_write_string("\r\n");
            break;
        case '\b':
            if (cmd_ptr) {
                cmd_ptr--;
                uart_transmit(byte);
            }
            break;
        case 21: // Ctrl-U
            cmd_ptr = 0;
            uart_write_string("\r\e[K");
            break;
        case '\r':
        case '\n':
            uart_write_string("\r\n");
            process_command();
            break;
        default:
            if (cmd_ptr < CMD_BUFFER_SIZE-1) {
                cmd_buffer[cmd_ptr++] = byte;
                uart_transmit(byte);
            }
            break;
        }
        if (cmd_ptr) {
            if (!file_locked)
                lock_file();
        } else {
            if (file_locked && uart_state == STATE_CLI)
                unlock_file();
        }
        break;
    case STATE_ZMODEM:
        zrx_byte(byte);
        if (!zmodem_active()) {
            uart_state = STATE_CLI;
            unlock_file();
        }
        break;
    }
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
    cmd_buffer = zmodem_init(&lfs, &file);

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

    file_locked = false;
    file_locked_request = false;
    file_open = false;
    int err = lfs_mount(&lfs, &cfg);
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    frame_timer_init();
    i2c_init();
    display_init();
    display_setposition(0, 0);

    display_command(0x20);
    display_command(0);

    screen_buf = frame1;
    display_buf = frame1;
    draw = false;

    /* This may be useful until a better way to
     * erase the entire filesystem exists
     */

//    speaker_on(2000);
//    spiflash_erase_chip();
//    speaker_off();

    start_display();

    bool usb_present = false;
    int norxframes = 0;

    frame_timer_on(150);

    while (1) {
        check_file_lock();
        if (draw) {
            draw = false;
            if (zmodem_active()) {
                zmodem_draw_progress(screen_buf);
            } else if (file_locked) {
                draw_cli(screen_buf);
            } else if (file_open) {
                draw_image(screen_buf);
            } else {
                draw_error(screen_buf);
            }

            display_buf = screen_buf;
            screen_buf = (screen_buf == frame1)?frame2:frame1;

            /* XXX Once we have a timer, we can improve on this */
            if (++norxframes > 1500) {
                  norxframes = 0;
                  if (uart_state == STATE_ZMODEM)
                      zmodem_timeout();
            }
        }

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

        while (uart_receive(&byte)) {
            norxframes = 0;
            process_byte(byte);
        }
    }

    return 0;
}
