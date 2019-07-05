/* display.h */

#ifndef DISPLAY_H
#define DISPLAY_H

bool display_command(uint8_t command);
bool display_data(uint8_t *data, int len);
void display_setposition(uint8_t col, uint8_t row);
bool display_init(void);

#endif /* DISPLAY_H */
