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

#define IDLE_FRAME_RATE 50
#define SYSTIME_FREQUENCY 1000

#define CMD_BUFFER_SIZE ZMODEM_BUFFER_SIZE

#define STATE_CLI 0
#define STATE_ZMODEM 1

int uart_state = STATE_CLI;
char *cmd_buffer;
int cmd_ptr = 0;
int arg_ptr = 0;

int contrast;
bool contrast_changed;
bool safe_start;

#define FIRST_FILE 2

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
    uart_init();
}

void usb_disconnected(void)
{
    LPC_IOCON->REG[IOCON_PIO1_7] = 0;    // Turn off pull up on TXD
    LPC_IOCON->REG[IOCON_PIO1_6] = 0;    // Turn off pull up on RXD
    LPC_IOCON->REG[IOCON_PIO0_7] = 0;    // Turn off pull up on CTS
    LPC_IOCON->REG[IOCON_PIO1_5] = 0;    // Turn off pull up on RTS
    uart_pause();
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

#define DEBOUNCE_TIME 10
#define FIRST_REPEAT_TIME 300
#define REPEAT_TIME 100

int button_event(void)
{
    static int buttons = 0;
    static uint32_t last_changed[5] = {0, 0, 0, 0, 0};
    static uint32_t last_event[5] = {0, 0, 0, 0, 0};
    int new_buttons = button_state();
    int changed = new_buttons ^ buttons;
    uint32_t now = systime_timer_get();

    for (int i = 0; i < 5; i++) {
        if (changed & (1<<i)) {
            if (now - last_changed[i] >= DEBOUNCE_TIME) {
                last_changed[i] = now;
                last_event[i] = now;
                buttons = (buttons & ~(1<<i)) | (new_buttons & (1<<i));
            }
        }
        if (buttons & (1<<i)) {
            int threshold = (last_changed[i] == last_event[i])?FIRST_REPEAT_TIME:REPEAT_TIME;
            if (now - last_event[i] >= threshold) {
                last_event[i] += threshold;
                changed |= (1<<i);
            }
        }
    }

    return buttons & changed;
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
    if (zmodem_waiting()) {
        for (int i = 0; i < 4; i++) {
            frame[128*7 + 64 + (i + 2)] = 255;
            frame[128*7 + 64 - (i + 2)] = 255;
            frame[128*6 + 64 + (i + 2)] = 255;
            frame[128*6 + 64 - (i + 2)] = 255;
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

#define ACTION_ONESHOT 0
#define ACTION_LOOP    1
#define ACTION_REVERSE 2

struct imageheader {
    int version;
    int data;
    int width;
    int height;
    int frame_rate;
    int scroll_rate;
    int action;
    int left_blank;
    int right_blank;
    int left_blank_pattern;
    int right_blank_pattern;
    int left_pause;
    int right_pause;
    int start;
};

struct imagestate {
    int pos;
};

struct videoheader {
    int version;
    int data;
    int width;
    int height;
    int frames;
    int frame_rate;
    int action;
    int start;
};

struct videostate {
    bool started;
    int frame;
};

struct {
    int size;
    union {
        struct {
            struct imageheader header;
            struct imagestate state;
        } image;
        struct {
            struct videoheader header;
            struct videostate state;
        } video;
    };
} filedata;

#define DISPLAY_NOTHING 0
#define DISPLAY_IMAGE 1
#define DISPLAY_VIDEO 2

int displaytype;
int dir_offset;

void set_frame_rate(int frame_rate)
{
    static int current_rate = 0;
    if (frame_rate == current_rate)
        return;
    current_rate = frame_rate;
    frame_timer_on(frame_rate);
//    uart_write_string("Frame rate set to ");
//    uart_write_int(frame_rate);
//    uart_write_string("\r\n");
}

void stop_display(void)
{
    displaytype = DISPLAY_NOTHING;
    if (file_open)
        lfs_file_close(&lfs, &file);
    file_open = 0;
    set_frame_rate(IDLE_FRAME_RATE);
}

void image_load(void)
{
    if (filedata.size < 8 + sizeof(filedata.image.header))
        goto fail;

    int ret = lfs_file_read(&lfs, &file, &filedata.image.header, sizeof(filedata.image.header));
    if (ret < 0)
        goto fail;

    if (filedata.image.header.height != 64)
        goto fail;

    if (filedata.image.header.width * 8 + filedata.image.header.data > filedata.size)
        goto fail;

    filedata.image.state.pos = filedata.image.header.start;
    set_frame_rate(filedata.image.header.frame_rate);

    displaytype = DISPLAY_IMAGE;

    return;

    fail:
        stop_display();
        return;
}

void video_load(void)
{
    if (filedata.size < 8 + sizeof(filedata.video.header))
        goto fail;

    int ret = lfs_file_read(&lfs, &file, &filedata.video.header, sizeof(filedata.video.header));
    if (ret < 0)
        goto fail;

    if (filedata.video.header.width != 128)
        goto fail;

    if (filedata.video.header.height != 64)
        goto fail;

    if (filedata.video.header.start < 0)
        goto fail;

    if (filedata.video.header.start >= filedata.video.header.frames)
        goto fail;

    if (filedata.video.header.frames * 1024 + filedata.video.header.data > filedata.size)
        goto fail;

    ret = lfs_file_seek(&lfs, &file, filedata.video.header.data + filedata.video.header.start * 1024, LFS_SEEK_SET);
    if (ret < 0)
        goto fail;

    filedata.video.state.started = true;
    filedata.video.state.frame = filedata.video.header.start;
    set_frame_rate(filedata.video.header.frame_rate);

    displaytype = DISPLAY_VIDEO;

    return;

    fail:
        stop_display();
        return;
}

void file_load(const char *filename)
{
    char magic[8];

    int ret = lfs_file_open(&lfs, &file, filename, LFS_O_RDONLY);
    if (ret)
        return;

    displaytype = DISPLAY_NOTHING;
    file_open = true;

    ret = lfs_file_size(&lfs, &file);
    if (ret < 0) {
        stop_display();
        return;
    }

    filedata.size = ret;

    ret = lfs_file_read(&lfs, &file, magic, 8);
    if (ret < 0) {
        stop_display();
        return;
    }

    if (memcmp(magic, "helloimg", 8) == 0)
        image_load();
    else if (memcmp(magic, "hellovid", 8) == 0)
        video_load();
    else
        stop_display();
}

int load_file_by_offset(int target)
{
    lfs_dir_t dir;
    if (safe_start)
        return 0;
    if (file_open)
        stop_display();
    int err = lfs_dir_open(&lfs, &dir, "/");
    if (err) {
        return -1;
    }
    struct lfs_info info;
    int last_fileno = 0;
    int fileno;
    for (fileno = 0; true; fileno++) {
        int res  = lfs_dir_read(&lfs, &dir, &info);
        if (res < 0) {
            lfs_dir_close(&lfs, &dir);
            return res;
        }
        if (res == 0) {
            if (target == FIRST_FILE) {
                lfs_dir_close(&lfs, &dir);
                return 0;
            }
            fileno = -1; // This will be incremented on continue
            if (target < 0) {
                target = last_fileno;
            } else {
                target = FIRST_FILE;
            }
            lfs_dir_rewind(&lfs, &dir);
            continue;
        }

        if (fileno < target)
            continue;

        last_fileno = fileno;

        if (fileno == target) {
            if (info.type != LFS_TYPE_REG) {
                target++;
                continue;
            }

            lfs_dir_close(&lfs, &dir);
            file_load(info.name);
            return fileno;
        }
    }
}

void next_file(void)
{
    dir_offset = load_file_by_offset(dir_offset + 1);
}

void prev_file(void)
{
    dir_offset--;
    if (dir_offset < FIRST_FILE)
        dir_offset = -1;
    int res = load_file_by_offset(dir_offset);
    if (dir_offset < FIRST_FILE)
        dir_offset = res;
}

void start_display(void)
{
    dir_offset = load_file_by_offset(dir_offset);
}

void contrast_up(void)
{
    contrast += 0x10;
    if (contrast > 0xff)
        contrast = 0xff;
    contrast_changed = true;
}

void contrast_down(void)
{
    contrast -= 0x10;
    if (contrast < 1)
        contrast = 1;
    contrast_changed = true;
}

void draw_image(uint8_t *frame)
{
    int offset = filedata.image.state.pos;

    int pos = filedata.image.state.pos;
    if (pos < -filedata.image.header.left_blank)
        pos = -filedata.image.header.left_blank;
    if (pos > filedata.image.header.width + filedata.image.header.right_blank - 128)
        pos = filedata.image.header.width + filedata.image.header.right_blank - 128;

    offset = pos;

    if (offset < 0)
        offset = 0;

    int datawidth;
    if (pos >= 0)
        datawidth = filedata.image.header.width - offset;
    else
        datawidth = 128 + pos;

    if (datawidth > 128)
        datawidth = 128;

    int datapos = 0;
    if (pos < 0)
        datapos = 128-datawidth;

    for (int j = 0; j < 8; j++) {
        int ret;
        for (int i = 0; i < datapos; i++)
            frame[i + j*128] = filedata.image.header.left_blank_pattern;
        for (int i = datapos + datawidth; i < 128; i++)
            frame[i + j*128] = filedata.image.header.right_blank_pattern;
        ret = lfs_file_seek(&lfs, &file, filedata.image.header.data + offset + j*filedata.image.header.width, LFS_SEEK_SET);
        if (ret < 0)
            goto fail;
        ret = lfs_file_read(&lfs, &file, frame + datapos + j*128, datawidth);
        if (ret < 0)
            goto fail;
    }
#if 0
    cursor_ptr = 0;
    write_int(norxframes);
#endif

#if 0
    cursor_ptr = 0;
    write_int(pos);
    cursor_ptr = 128;
    write_int(datawidth);
    cursor_ptr = 128*2;
    write_int(datapos);
#endif

    filedata.image.state.pos += filedata.image.header.scroll_rate;
    if (filedata.image.state.pos < 0-(filedata.image.header.left_pause + filedata.image.header.left_blank)) {
        filedata.image.state.pos = -filedata.image.header.left_blank;
        filedata.image.header.scroll_rate = -filedata.image.header.scroll_rate;
    }
    if (filedata.image.state.pos > filedata.image.header.width - 128 + filedata.image.header.right_pause + filedata.image.header.right_blank) {
        switch (filedata.image.header.action) {
        case ACTION_ONESHOT:
        default:
            stop_display();
            break;
        case ACTION_LOOP:
            filedata.image.state.pos = -filedata.image.header.left_blank;
            break;
        case ACTION_REVERSE:
            filedata.image.state.pos = filedata.image.header.width - 128 + filedata.image.header.right_blank;
            filedata.image.header.scroll_rate = -filedata.image.header.scroll_rate;
            break;
        }
    }

    return;

    fail:
        stop_display();
        return;
}

void draw_video(uint8_t *frame)
{
    int ret;
    if (!filedata.video.state.started) {
        ret = lfs_file_seek(&lfs, &file, filedata.video.header.data, LFS_SEEK_SET);
        if (ret < 0)
            goto fail;
        filedata.video.state.started = true;
        filedata.video.state.frame = 0;
    }
    ret = lfs_file_read(&lfs, &file, frame, 1024);
    if (ret < 0)
        goto fail;

    filedata.video.state.frame++;
    if (filedata.video.state.frame >= filedata.video.header.frames) {
        /* XXX implement actions here */
        filedata.video.state.started = false;
    }
    return;

    fail:
        stop_display();
        return;
}

void FRAME_HANDLER(void)
{
    FRAME_BASE->IR = 15;
    if (display_busy())
        return;
    display_start(display_buf, 1024, contrast_changed?contrast:-1);
    contrast_changed = false;
    draw = true;
}

void check_zmodem(void)
{
    if (!zmodem_active()) {
        uart_state = STATE_CLI;
        timeout_timer_off();
        unlock_file();
    }
}

void TIMEOUT_HANDLER(void)
{
    TIMEOUT_BASE->IR = 15;
    zmodem_timeout();
    check_zmodem();
}

void check_file_lock(void)
{
    if (file_locked_request) {
        if (file_open) {
            stop_display();
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
    timeout_timer_on(ZMODEM_TIMEOUT);
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

void cmd_format(void)
{
    uart_write_string("Formatting filesystem... ");
    lfs_unmount(&lfs);
    lfs_format(&lfs, &cfg);
    int err = lfs_mount(&lfs, &cfg);
    if (err < 0)
        uart_write_string("failed\r\n");
    else
        uart_write_string("done\r\n");
}

void cmd_reset(void)
{
    NVIC_SystemReset();
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
    {"format", &cmd_format},
    {"reset", &cmd_reset},
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
        timeout_timer_reset();
        zrx_byte(byte);
        check_zmodem();
        break;
    }
}

void uart_rxhandler(uint8_t byte)
{
    process_byte(byte);
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

    uart_init();
    cmd_buffer = zmodem_init(&lfs, &file);

    speaker_init();
    buttons_init();

#if 1
    speaker_on(500);
    delay(20);
    speaker_off();
#endif

    delay(500);

#if 1
    speaker_on(750);
    delay(20);
    speaker_off();
#endif

//    uart_write_string("Hello\r\n");

#if 1
    delay(20);
    speaker_on(1000);
    delay(20);
    speaker_off();
#endif

    contrast = 0x7f;
    contrast_changed = false;

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
    timeout_timer_init();
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

//    speaker_on(1000);
//    spiflash_erase_chip();
//    speaker_off();

    safe_start = (button_state() & BUTTON_CENTRE)?true:false;

    set_frame_rate(IDLE_FRAME_RATE);

    dir_offset = FIRST_FILE;
    start_display();

    bool usb_present = false;

    while (1) {
        check_file_lock();
        if (draw) {
            draw = false;
            if (zmodem_active()) {
                zmodem_draw_progress(screen_buf);
            } else if (file_locked || safe_start) {
                draw_cli(screen_buf);
            } else if (file_open) {
                switch (displaytype) {
                case DISPLAY_IMAGE:
                    draw_image(screen_buf);
                    break;
                case DISPLAY_VIDEO:
                    draw_video(screen_buf);
                    break;
                default:
                    draw_error(screen_buf);
                    break;
                }
            } else {
                draw_error(screen_buf);
            }

            display_buf = screen_buf;
            screen_buf = (screen_buf == frame1)?frame2:frame1;
        }

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
