/* uart.c */

#include "hello.h"
#include "uart.h"

#define UART_PCLK 48000000

static bool suspended;

void uart_init(void)
{
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_UART0);
    Chip_SYSCTL_DeassertPeriphReset(RESET_UART0);
    LPC_SYSCTL->USARTCLKDIV = 1;

    LPC_USART->FCR = UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2;
    LPC_USART->LCR = UART_LCR_WLEN8;
    LPC_USART->MCR = UART_MCR_AUTO_RTS_EN | UART_MCR_AUTO_CTS_EN;
    LPC_USART->ACR = UART_ACR_START | UART_ACR_MODE | UART_ACR_AUTO_RESTART;

    suspended = false;
}

void uart_transmit(uint8_t byte)
{
    while (!(LPC_USART->LSR & UART_LSR_THRE))
        ;
    LPC_USART->THR = byte;
}

bool uart_receive(uint8_t *byte)
{
    if (suspended)
        return false;
    if (LPC_USART->LSR & UART_LSR_RDR) {
        *byte = LPC_USART->RBR;
        return true;
    } else
        return false;
}

void uart_pause(void)
{
    suspended = true;
}

void uart_resume(void)
{
    suspended = false;
}
