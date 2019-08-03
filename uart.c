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
    LPC_USART->IER = UART_IER_RBRINT | UART_IER_RLSINT;

    suspended = false;
    NVIC_EnableIRQ(UART0_IRQn);
}

void UART0_IRQHandler(void)
{
    int iir = LPC_USART->IIR;
    switch (iir & UART_IIR_INTID_MASK) {
    case UART_IIR_INTID_RLS:
        if (LPC_USART->LSR & (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE))
            LPC_USART->ACR = UART_ACR_START | UART_ACR_MODE | UART_ACR_AUTO_RESTART;
        break;
    case UART_IIR_INTID_RDA:
    case UART_IIR_INTID_CTI:
        while ((LPC_USART->LSR & UART_LSR_RDR) && !suspended)
            uart_rxhandler(LPC_USART->RBR);
        break;
    case UART_IIR_INTID_THRE:
    case UART_IIR_INTID_MODEM:
        /* Do nothing */
        break;
    }
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
    NVIC_DisableIRQ(UART0_IRQn);
}

void uart_resume(void)
{
    suspended = false;
    NVIC_EnableIRQ(UART0_IRQn);
}
