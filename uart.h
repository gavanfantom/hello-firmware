/* uart.h */

#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_transmit(uint8_t byte);
bool uart_receive(uint8_t *byte);
void uart_pause(void);
void uart_resume(void);
void uart_rxhandler(uint8_t byte);

#endif /* UART_H */
