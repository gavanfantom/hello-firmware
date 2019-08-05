/* fault.c */

#include "hello.h"
#include "write.h"

void HardFault_Handler( void ) __attribute__( ( naked ) );
void HardFault_Handler(void)
{
    __asm volatile
    (
        " mrs r0, msp                                               \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    uart_write_string("FAULT\r\n");
    uart_write_hex(r0);
    uart_write_string("\r\n");
    uart_write_hex(r1);
    uart_write_string("\r\n");
    uart_write_hex(r2);
    uart_write_string("\r\n");
    uart_write_hex(r3);
    uart_write_string("\r\n");
    uart_write_hex(r12);
    uart_write_string("\r\n");
    uart_write_hex(lr);
    uart_write_string("\r\n");
    uart_write_hex(pc);
    uart_write_string("\r\n");
    uart_write_hex(psr);
    uart_write_string("\r\n");

    for( ;; );
}

