/* font.h */

#ifndef FONT_H
#define FONT_H

extern uint8_t *screen_buf;
extern int cursor_ptr;

void font_putchar(char ch);
void font_putchar_large(char ch);

#endif /* FONT_H */
