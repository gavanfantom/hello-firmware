/* menu.c */

#include "hello.h"
#include "menu.h"
#include "button.h"
#include "write.h"
#include "font.h"
#include "version.h"
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

#define SELECTION_FILE_ACTION_SHOW    0
#define SELECTION_FILE_ACTION_DELETE  1

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

typedef void (*draw_handler)(uint8_t *frame);
typedef void (*action_handler)(void);

action_handler menu_actions[MENU_MAX][BUTTONS];
draw_handler      menu_draw[MENU_MAX];

void menu_action_exit(void);
char *menu_find_file(lfs_dir_t *dir, struct lfs_info *info, int selection);
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
}

void invert_line(uint8_t *frame, int line)
{
    for (int i = 0; i < 128; i++)
        frame[line*128 + i] ^= 0xff;
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
    write_string("hello version " HELLO_VERSION "\n");
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

void menu_file_select(void)
{
}

char *menu_read_dirent(lfs_dir_t *dir, struct lfs_info *info)
{
    while (1) {
        int res = lfs_dir_read(&fs_lfs, dir, info);
        if (res != 1)
            return NULL;
        if (info->type != LFS_TYPE_REG)
            continue;
//        if (info->name[0] == '.')
//            continue;
        return info->name;
    }
}

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
        menu_read_dirent(&dir, &info);
    }
    for (int i = 0; i < 8; i++) {
        char *name = menu_read_dirent(&dir, &info);
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

void draw_menu_file_actions(uint8_t *frame)
{
    lfs_dir_t dir;
    struct lfs_info info;
    char *name = menu_find_file(&dir, &info, file);
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
    write_string("Delete");
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
    lfs_dir_t dir;
    struct lfs_info info;
    if (lfs_dir_open(&fs_lfs, &dir, "/"))
        return;
    selection = 0;
    selection_min = 0;
    selection_max = -1;
    line_offset = 0;
    line_scroll_rate = 1;
    line_scroll_count = SCROLL_COUNT_END;
    while (menu_read_dirent(&dir, &info))
        selection_max++;
    lfs_dir_close(&fs_lfs, &dir);
    if (selection_max >= 0)
        menu = MENU_FILE;
}

void menu_return_to_file(void)
{
    int offset = line_offset;
    menu_enter_file();
    selection = file;
    line_offset = offset;
}

void menu_enter_about(void)
{
    menu = MENU_ABOUT;
    selection = 0;
    selection_min = 0;
    selection_max = 0;
}

void menu_enter_edit(void)
{
}

void menu_enter_settings(void)
{
    menu = MENU_SETTINGS;
    selection = 0;
    selection_min = 0;
    selection_max = 3;
}

char *menu_find_file(lfs_dir_t *dir, struct lfs_info *info, int fileno)
{
    char *name = NULL;
    if (lfs_dir_open(&fs_lfs, dir, "/"))
        return NULL;
    for (int i = 0; i <= fileno; i++) {
        name = menu_read_dirent(dir, info);
        if (!name)
            break;
    }
    lfs_dir_close(&fs_lfs, dir);
    return name;
}

void menu_select_file(void)
{
    lfs_dir_t dir;
    struct lfs_info info;
    const char *name = menu_find_file(&dir, &info, selection);
    if (name) {
        file_load_update_offset(name);
        menu_action_exit();
    }
}

void menu_delete_file(void)
{
    lfs_dir_t dir;
    struct lfs_info info;
    const char *name = menu_find_file(&dir, &info, file);
    lfs_remove(&fs_lfs, name);
}

void menu_select_file_actions(void)
{
    menu = MENU_FILE_ACTIONS;
    file = selection;
    selection = 0;
    selection_min = 0;
    selection_max = 1;
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
};

