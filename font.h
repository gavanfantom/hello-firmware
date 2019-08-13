/* font.h */

#ifndef FONT_H
#define FONT_H

extern int cursor_ptr;

#define FONT_LARGE_PIXEL_WIDTH 5
#define FONT_LARGE_BLANK_WIDTH 3

#define FONT_LARGE_TOTAL_WIDTH (5*(FONT_LARGE_PIXEL_WIDTH) + (FONT_LARGE_BLANK_WIDTH))
#define FONT_LARGE_CHARACTERS_PER_SCREEN (128 / FONT_LARGE_TOTAL_WIDTH)

#define FONT_CHAR_MIN 32
#define FONT_CHAR_MAX 127

void font_putchar(char ch);
void font_putchar_large(char ch);
void font_setpos(int column, int row);
bool font_getpixel(char ch, int column, int row);

#endif /* FONT_H */
