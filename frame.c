/* frame.c */

#include "hello.h"
#include "frame.h"
#include "display.h"
#include "file.h"
#include "lfs.h"
#include "fs.h"
#include "zmodem.h"
#include "write.h"
#include "font.h"
#include "timer.h"
#include "lock.h"
#include "menu.h"
#include "settings.h"
#include "battery.h"

uint8_t frame1[1024];
uint8_t frame2[1024];
uint8_t *screen_buf;
uint8_t *display_buf;
bool draw;

void frame_init(void)
{
    screen_buf = frame1;
    display_buf = frame1;
    draw = false;
    display_init();
    display_setposition(0, 0);
    display_command(0x20);
    display_command(0);
    frame_timer_init();
    set_frame_rate(IDLE_FRAME_RATE);
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

#define DISPLAY_NOTHING 0
#define DISPLAY_IMAGE 1
#define DISPLAY_VIDEO 2

int displaytype;

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
        ret = lfs_file_seek(&fs_lfs, &fs_file, filedata.image.header.data + offset + j*filedata.image.header.width, LFS_SEEK_SET);
        if (ret < 0)
            goto fail;
        ret = lfs_file_read(&fs_lfs, &fs_file, frame + datapos + j*128, datawidth);
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
        ret = lfs_file_seek(&fs_lfs, &fs_file, filedata.video.header.data, LFS_SEEK_SET);
        if (ret < 0)
            goto fail;
        filedata.video.state.started = true;
        filedata.video.state.frame = 0;
    }
    ret = lfs_file_read(&fs_lfs, &fs_file, frame, 1024);
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

#define BATTERY_PIXELS 20
#define BATTERY_WIDTH (BATTERY_PIXELS + 6)

void draw_battery(uint8_t *frame)
{
    int battery = battery_status();
    if (battery < BATTERY_EMPTY)
        battery = BATTERY_EMPTY;
    if (battery > BATTERY_FULL)
        battery = BATTERY_FULL;
    battery = battery - BATTERY_EMPTY;
    battery = battery * BATTERY_PIXELS / (BATTERY_FULL - BATTERY_EMPTY);
    int settings = settings_battery();
    if (settings == BATTERY_OFF)
        return;
    if (settings == BATTERY_WHENLOW)
        if (battery < BATTERY_THRESHOLD)
            return;
    int offset = 128 - BATTERY_WIDTH;
    frame[offset++] = 0x3c;
    frame[offset++] = 0x24;
    frame[offset++] = 0xe7;
    frame[offset++] = 0x81;
    for (int i = 0; i < BATTERY_PIXELS; i++) {
        if (i < (BATTERY_PIXELS - battery))
            frame[offset + i] = 0x81;
        else
            frame[offset + i] = 0xbd;
    }
    offset += BATTERY_PIXELS;
    frame[offset++] = 0x81;
    frame[offset++] = 0xff;
}

void FRAME_HANDLER(void)
{
    static int contrast = 0;
    FRAME_BASE->IR = 15;
    if (display_busy())
        return;
    int new_contrast = settings_brightness();
    if (new_contrast == contrast)
        new_contrast = -1;
    else
        contrast = new_contrast;
    display_start(display_buf, 1024, new_contrast);
    draw = true;
}

void frame_update(void)
{
    if (!draw)
        return;
    draw = false;
    if (zmodem_active()) {
        zmodem_draw_progress(screen_buf);
    } else if (file_locked || safe_start) {
        draw_cli(screen_buf);
    } else if (menu != MENU_NONE) {
        draw_menu(screen_buf);
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
    draw_battery(screen_buf);
    battery_poll();

    display_buf = screen_buf;
    screen_buf = (screen_buf == frame1)?frame2:frame1;
}

