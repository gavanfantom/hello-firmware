/* menu.h */

#ifndef MENU_H
#define MENU_H

#define MENU_NONE         0
#define MENU_MAIN         1
#define MENU_ABOUT        2
#define MENU_FILE         3
#define MENU_FILE_ACTIONS 4
#define MENU_SETTINGS     5

#define MENU_MAX          5

extern int menu;

void menu_init(void);
void draw_menu(uint8_t *frame);
void menu_action(int pressed);
void menu_enter(void);

#endif /* MENU_H */
