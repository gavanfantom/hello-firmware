/* write.h */

#ifndef WRITE_H
#define WRITE_H

void write_string(char *s);
void write_string_large(char *s);
void write_int(int val);
void uart_write_int(int val);
void uart_write_hex(uint32_t val);
void uart_write_string(const char *s);
void clear_screen(void);

#endif /* WRITE_H */
