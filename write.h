/* write.h */

#ifndef WRITE_H
#define WRITE_H

extern int cursor_ptr;

void write_string(const char *s);
void write_string_large(const char *s);
void write_int(int val);
void uart_write_int(int val);
void uart_write_hex(uint32_t val);
void uart_write_string(const char *s);
void clear_screen(void);

#endif /* WRITE_H */
