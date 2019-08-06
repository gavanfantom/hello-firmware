/* font.h */

#ifndef FONT_H
#define FONT_H

extern int cursor_ptr;

void font_putchar(char ch);
void font_putchar_large(char ch);
void font_setpos(int column, int row);

#endif /* FONT_H */
