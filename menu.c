/* menu.c */

#include "hello.h"
#include "menu.h"
#include "button.h"
#include "write.h"
#include "font.h"
#include "settings.h"
#include "lfs.h"
#include "fs.h"
#include "frame.h"
#include "file.h"
#include <string.h>

#define SELECTION_MAIN_FILE     0
#define SELECTION_MAIN_ABOUT    1
#define SELECTION_MAIN_EDIT     2
#define SELECTION_MAIN_SETTINGS 3

#define SELECTION_SETTINGS_BEEP       0
#define SELECTION_SETTINGS_BRIGHTNESS 1
#define SELECTION_SETTINGS_BATTERY    2
#define SELECTION_SETTINGS_SAVE       3

#define SELECTION_FILE_ACTION_SHOW         0
#define SELECTION_FILE_ACTION_SET_DEFAULT  1
#define SELECTION_FILE_ACTION_DELETE       2

#define SELECTION_EDIT_ACTION_SAVE    0
#define SELECTION_EDIT_ACTION_DISCARD 1

#define SCROLL_COUNT       (IDLE_FRAME_RATE / 3)
#define SCROLL_COUNT_END   (IDLE_FRAME_RATE * 3)

int menu;
int selection;
int selection_min;
int selection_max;
int file;
int offset;
int line_offset;
int line_scroll_rate;
int line_scroll_count;
int character;

#define EDIT_BUFFER_LEN 128
uint8_t edit_buffer[EDIT_BUFFER_LEN];

typedef void (*draw_handler)(uint8_t *frame);
typedef void (*action_handler)(void);

action_handler menu_actions[MENU_MAX][BUTTONS];
draw_handler      menu_draw[MENU_MAX];

void menu_action_exit(void);
void menu_return_to_file(void);

void menu_init(void)
{
    menu = MENU_MAIN;
    selection = 0;
    selection_min = 0;
    selection_max = 0;
    offset = 0;
    line_offset = 0;
    line_scroll_rate = 1;
    line_scroll_count = SCROLL_COUNT_END;
    character = 0;
}

void invert_line(uint8_t *frame, int line)
{
    for (int i = 0; i < 128; i++)
        frame[line*128 + i] ^= 0xff;
}

void invert_columns(uint8_t *frame, int start, int width)
{
    if (start < 0) {
        width += start;
        start = 0;
    }
    if (start >= 128)
        return;
    if (start + width >= 128) {
        width = 127 - start;
    }
    for (int i = start; i < start + width; i++)
        for (int j = 0; j < 8; j++)
            frame[j*128 + i] ^= 0xff;
}

void draw_menu_main(uint8_t *frame)
{
    clear_screen();
    font_setpos(6, 0);
    write_string("h e l l o");
    font_setpos(2, 2);
    write_string("Files");
    font_setpos(2, 3);
    write_string("About");
    font_setpos(2, 4);
    write_string("Edit name");
    font_setpos(2, 5);
    write_string("Settings");
    invert_line(frame, selection+2);
}

void draw_menu_about(uint8_t *frame)
{
    clear_screen();
    write_string("hello " HELLO_VERSION "\n");
    write_string("Your name in style\n");
    write_string("Use the serial port\n");
    write_string("8N1 RTS/CTS press >a<\n");
    write_string("\n");
    write_string("Convert imgs and vids\n");
    write_string("then send with ZMODEM\n");
    write_string("       coolfactor.org\n");
    invert_line(frame, 0);
    invert_line(frame, 1);
    invert_line(frame, 7);
}

#define LINE_MAX 21

void draw_menu_file(uint8_t *frame)
{
    lfs_dir_t dir;
    struct lfs_info info;
    if (lfs_dir_open(&fs_lfs, &dir, "/")) {
        menu_enter();
        return;
    }
    screen_buf = frame;
    clear_screen();
    if (selection < offset)
        offset = selection;
    if (selection > offset + 7)
        offset = selection - 7;
    for (int i = 0; i < offset; i++) {
        file_read_dirent(&dir, &info, false);
    }
    for (int i = 0; i < 8; i++) {
        char *name = file_read_dirent(&dir, &info, false);
        if (!name)
            break;
        int len = strlen(name);
        if ((i == selection) && (len > LINE_MAX)) {
            if (line_scroll_count == 0) {
                line_offset += line_scroll_rate;
                line_scroll_count = SCROLL_COUNT;
                if ((line_offset + LINE_MAX) > len) {
                    line_offset--;
                    line_scroll_rate = -line_scroll_rate;
                    line_scroll_count = SCROLL_COUNT_END;
                }
                if (line_offset < 0) {
                    line_offset++;
                    line_scroll_rate = -line_scroll_rate;
                    line_scroll_count = SCROLL_COUNT_END;
                }
            } else
                line_scroll_count--;
            name += line_offset;
        }
        if (len - line_offset > LINE_MAX)
            name[LINE_MAX] = '\0';
        write_string(name);
        write_string("\n");
    }
    invert_line(frame, selection - offset);
    lfs_dir_close(&fs_lfs, &dir);
}

void draw_menu_edit(uint8_t *frame)
{
    if (selection < offset)
        offset = selection;
    if (selection > offset + FONT_LARGE_CHARACTERS_PER_SCREEN - 1)
        offset = selection - FONT_LARGE_CHARACTERS_PER_SCREEN + 1;
    screen_buf = frame;
    clear_screen();
    for (int i = 0; i < FONT_LARGE_CHARACTERS_PER_SCREEN; i++) {
        if ((offset + i) <= selection_max) {
            cursor_ptr = i * FONT_LARGE_TOTAL_WIDTH;
            font_putchar_large(edit_buffer[offset + i]);
        }
    }
    invert_columns(frame, (selection - offset) * FONT_LARGE_TOTAL_WIDTH, FONT_LARGE_TOTAL_WIDTH);
}

void draw_menu_file_actions(uint8_t *frame)
{
    lfs_dir_t dir;
    struct lfs_info info;
    char *name = file_find(&dir, &info, file, false);
    if (!name) {
        menu_return_to_file();
        return;
    }
    int len = strlen(name);
    if (len > LINE_MAX) {
        name[LINE_MAX] = '\0';
        len = LINE_MAX;
    }
    clear_screen();
    font_setpos((LINE_MAX-len)/2, 0);
    write_string(name);
    font_setpos(2, 2);
    write_string("Show");
    font_setpos(2, 3);
    write_string("Set as default");
    font_setpos(2, 4);
    write_string("Delete");
    invert_line(frame, selection+2);
}

void draw_menu_edit_actions(uint8_t *frame)
{
    char name[LINE_MAX+1];
    memset(name, 0, LINE_MAX+1);
    strncpy(name, (char *)edit_buffer, LINE_MAX);
    screen_buf = frame;
    clear_screen();
    font_setpos(0, 0);
    write_string(name);
    font_setpos(2, 2);
    write_string("Save");
    font_setpos(2, 3);
    write_string("Discard");
    invert_line(frame, selection+2);
}

const char *menu_battery_text(int state)
{
    switch (state) {
    case BATTERY_ON:
        return "On";
    case BATTERY_WHENLOW:
        return "When Low";
    case BATTERY_OFF:
        return "Off";
    default:
        return "???";
    }
}

void draw_menu_settings(uint8_t *frame)
{
    clear_screen();
    font_setpos(2, 0);
    write_string("s e t t i n g s");
    font_setpos(2, 2);
    write_string("Beep       ");
    write_string(settings_beep()?"On":"Off");
    font_setpos(2, 3);
    write_string("Brightness ");
    write_int(100*settings_brightness()/255);
    font_setpos(2, 4);
    write_string("Battery    ");
    write_string(menu_battery_text(settings_battery()));
    font_setpos(2, 5);
    write_string("Save");
    invert_line(frame, selection+2);
}

void menu_action_none(void)
{
}

void menu_enter_file(void)
{
    int max = file_count(false) - 1;
    if (max >= 0) {
        selection = 0;
        selection_min = 0;
        selection_max = max;
        line_offset = 0;
        line_scroll_rate = 1;
        line_scroll_count = SCROLL_COUNT_END;
        menu = MENU_FILE;
    }
}

void menu_enter_edit(void)
{
    selection = 0;
    selection_min = 0;
    selection_max = 0;
    offset = 0;
    memset(edit_buffer, ' ', EDIT_BUFFER_LEN);
    character = 'A';
    edit_buffer[0] = character;
    menu = MENU_EDIT;
}

void menu_return_to_file(void)
{
    int offset = line_offset;
    menu_enter_file();
    selection = file;
    line_offset = offset;
}

void menu_return_to_edit(void)
{
    offset = 0;
    selection = 0;
    selection_min = 0;
    selection_max = 0;
    for (int i = 0; i < EDIT_BUFFER_LEN; i++) {
        if (edit_buffer[i] != ' ')
            selection_max = i;
    }
    character = edit_buffer[selection];
    menu = MENU_EDIT;
}

void menu_enter_about(void)
{
    menu = MENU_ABOUT;
    selection = 0;
    selection_min = 0;
    selection_max = 0;
}

void menu_enter_settings(void)
{
    menu = MENU_SETTINGS;
    selection = 0;
    selection_min = 0;
    selection_max = 3;
}

void menu_select_file(void)
{
    lfs_dir_t dir;
    struct lfs_info info;
    const char *name = file_find(&dir, &info, selection, false);
    if (name) {
        file_load_update_offset(name);
        menu_action_exit();
    }
}

void menu_select_file_default(void)
{
    lfs_dir_t dir;
    struct lfs_info info;
    const char *name = file_find(&dir, &info, selection, false);
    if (name) {
        file_set_default(name);
        settings_save();
        menu_action_exit();
    }
}

void menu_delete_file(void)
{
    lfs_dir_t dir;
    struct lfs_info info;
    const char *name = file_find(&dir, &info, file, false);
    lfs_remove(&fs_lfs, name);
}

void menu_select_file_actions(void)
{
    menu = MENU_FILE_ACTIONS;
    file = selection;
    selection = 0;
    selection_min = 0;
    selection_max = 2;
}

void menu_select_file_do_action(void)
{
    switch (selection) {
    case SELECTION_FILE_ACTION_SHOW:
        selection = file;
        menu_select_file();
        /* Just in case the menu is still around... */
        selection = SELECTION_FILE_ACTION_SHOW;
        break;
    case SELECTION_FILE_ACTION_SET_DEFAULT:
        selection = file;
        menu_select_file_default();
        /* Just in case the menu is still around... */
        selection = SELECTION_FILE_ACTION_SHOW;
        break;
    case SELECTION_FILE_ACTION_DELETE:
        menu_delete_file();
        menu_enter_file();
        break;
    }
}

void menu_select_settings(void)
{
    switch (selection) {
    case SELECTION_SETTINGS_SAVE:
        settings_save();
        menu_action_exit();
        break;
    }
}

void menu_select_settings_actions(void)
{
    switch (selection) {
    case SELECTION_SETTINGS_BEEP:
        settings_change_beep();
        break;
    case SELECTION_SETTINGS_BRIGHTNESS:
        settings_change_brightness_up();
        break;
    case SELECTION_SETTINGS_BATTERY:
        settings_change_battery();
        break;
    }
}

#define NAMELEN 128
#define ITERATION_MAX 100

void menu_edit_action_save(void)
{
    char name[NAMELEN];
    int max = 0;
    for (int i = 0; i < EDIT_BUFFER_LEN; i++) {
        if (edit_buffer[i] != ' ')
            max = i;
    }
    if (max == 0) {
        menu_enter();
        return;
    }
    int namelen = max+1;
    if (namelen > NAMELEN-1)
        namelen = NAMELEN-1;
    name[NAMELEN-1] = '\0';
    strncpy(name, (char *)edit_buffer, namelen);
    int len = strlen(name);
    for (int i = 0; i < ITERATION_MAX; i++) {
        if (i > 0) {
            char result[12];
            write_int_string(i, result);
            int l = strlen(result);
            int p = len;
            if (p+l+1 >= NAMELEN-1)
                p = NAMELEN-l-2;
            name[p] = '.';
            strcpy((char *)name+p+1, (char *)result);
        }
        int err = lfs_file_open(&fs_lfs, &fs_file, name, LFS_O_CREAT | LFS_O_EXCL);
        if (err == LFS_ERR_EXIST)
            continue;
        if (err < 0)
            break;
        uint8_t magic[] = "helloimg";
        struct imageheader hdr;
        hdr.version = 0;
        hdr.data = sizeof(hdr) + 8;
        hdr.width = (max+1) * FONT_LARGE_TOTAL_WIDTH;
        hdr.height = 64;
        hdr.frame_rate = 50;
        hdr.scroll_rate = 1;
        hdr.action = ACTION_REVERSE;
        hdr.left_blank = 0;
        hdr.right_blank = 0;
        hdr.left_blank_pattern = 0;
        hdr.right_blank_pattern = 0;
        hdr.left_pause = 20;
        hdr.right_pause = 20;
        hdr.start = 0;
        lfs_file_write(&fs_lfs, &fs_file, magic, 8);
        lfs_file_write(&fs_lfs, &fs_file, &hdr, sizeof(hdr));
        for (int i = 0; i < 8; i++) {
            for (int c = 0; c <= max; c++) {
                for (int j = 0; j < 5; j++) {
                    uint8_t value = font_getpixel(edit_buffer[c], j, i)?255:0;
                    uint8_t data[FONT_LARGE_PIXEL_WIDTH];
                    memset(data, value, FONT_LARGE_PIXEL_WIDTH);
                    lfs_file_write(&fs_lfs, &fs_file, data, FONT_LARGE_PIXEL_WIDTH);
                }
                uint8_t data[FONT_LARGE_BLANK_WIDTH];
                memset(data, 0, FONT_LARGE_BLANK_WIDTH);
                lfs_file_write(&fs_lfs, &fs_file, data, FONT_LARGE_BLANK_WIDTH);
            }
        }
        /* XXX actually write the image data here */
        lfs_file_close(&fs_lfs, &fs_file);
        file_load_update_offset(name);
        menu_action_exit();
        return;
    }
}

void menu_select_edit_actions(void)
{
    menu = MENU_EDIT_ACTIONS;
    selection = 0;
    selection_min = 0;
    selection_max = 1;
}

void menu_select_edit_do_action(void)
{
    switch (selection) {
    case SELECTION_EDIT_ACTION_SAVE:
        menu_edit_action_save();
        break;
    case SELECTION_EDIT_ACTION_DISCARD:
        menu_enter();
        selection = SELECTION_MAIN_EDIT;
        break;
    }
}

void menu_action_exit(void)
{
    menu = MENU_NONE;
}

void menu_select_up_file(void)
{
    if (selection > selection_min) {
        selection--;
        line_offset = 0;
        line_scroll_rate = 1;
        line_scroll_count = SCROLL_COUNT_END;
    }
}

void menu_select_down_file(void)
{
    if (selection < selection_max) {
        selection++;
        line_offset = 0;
        line_scroll_rate = 1;
        line_scroll_count = SCROLL_COUNT_END;
    }
}

void menu_select_up(void)
{
    if (selection > selection_min)
        selection--;
}

void menu_select_down(void)
{
    if (selection < selection_max)
        selection++;
}

void menu_select_edit_left(void)
{
    if (selection > selection_min) {
        selection--;
        character = edit_buffer[selection];
    }
}

void menu_select_edit_right(void)
{
    if (selection < selection_max) {
        selection++;
        character = edit_buffer[selection];
    } else if (selection_max < (EDIT_BUFFER_LEN-1)) {
        selection++;
        selection_max++;
        edit_buffer[selection] = character;
    }
}

void menu_select_edit_up(void)
{
    character++;
    if (character > FONT_CHAR_MAX)
        character = FONT_CHAR_MIN;
    edit_buffer[selection] = character;
}

void menu_select_edit_down(void)
{
    character--;
    if (character < FONT_CHAR_MIN)
        character = FONT_CHAR_MAX;
    edit_buffer[selection] = character;
}

void menu_select_main(void)
{
    switch (selection) {
    case SELECTION_MAIN_ABOUT:
        menu_enter_about();
        break;
    case SELECTION_MAIN_FILE:
        menu_enter_file();
        break;
    case SELECTION_MAIN_EDIT:
        menu_enter_edit();
        break;
    case SELECTION_MAIN_SETTINGS:
        menu_enter_settings();
        break;
    }
}

void menu_enter(void)
{
    menu = MENU_MAIN;
    selection = 0;
    selection_min = 0;
    selection_max = 3;
}

void draw_menu(uint8_t *frame)
{
    if ((menu < 1) || (menu > MENU_MAX))
        return;
    menu_draw[menu-1](frame);
}

void menu_process_action(int button)
{
    if ((button < 0) || (button >= BUTTONS))
        return;
    if ((menu < 1) || (menu > MENU_MAX))
        return;
    menu_actions[menu-1][button]();
}

void menu_action(int pressed)
{
    if (pressed & BUTTON_UP)
        menu_process_action(0);
    if (pressed & BUTTON_DOWN)
        menu_process_action(1);
    if (pressed & BUTTON_LEFT)
        menu_process_action(2);
    if (pressed & BUTTON_RIGHT)
        menu_process_action(3);
    if (pressed & BUTTON_CENTRE)
        menu_process_action(4);
}

draw_handler menu_draw[MENU_MAX]= {
    draw_menu_main,
    draw_menu_about,
    draw_menu_file,
    draw_menu_file_actions,
    draw_menu_settings,
    draw_menu_edit,
    draw_menu_edit_actions,
};

action_handler menu_actions[MENU_MAX][BUTTONS] = {
    /* MENU_MAIN */
    {
        menu_select_up,
        menu_select_down,
        menu_action_exit,
        menu_action_none,
        menu_select_main,
    },
    /* MENU_ABOUT */
    {
        menu_select_up,
        menu_select_down,
        menu_enter,
        menu_action_none,
        menu_action_none,
    },
    /* MENU_FILE */
    {
        menu_select_up_file,
        menu_select_down_file,
        menu_enter,
        menu_select_file_actions,
        menu_select_file,
    },
    /* MENU_FILE_ACTIONS */
    {
        menu_select_up,
        menu_select_down,
        menu_return_to_file,
        menu_action_none,
        menu_select_file_do_action,
    },
    /* MENU_SETTINGS */
    {
        menu_select_up,
        menu_select_down,
        menu_enter,
        menu_select_settings_actions,
        menu_select_settings,
    },
    /* MENU_EDIT */
    {
        menu_select_edit_up,
        menu_select_edit_down,
        menu_select_edit_left,
        menu_select_edit_right,
        menu_select_edit_actions,
    },
    /* MENU_EDIT_ACTIONS */
    {
        menu_select_up,
        menu_select_down,
        menu_return_to_edit,
        menu_action_none,
        menu_select_edit_do_action,
    },
};

