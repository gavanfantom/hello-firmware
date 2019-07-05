/* hello.h */

#ifndef HELLO_H
#define HELLO_H

#include "chip.h"

#define DATAREG (0x3ffc >> 2)
#define SET_LED(x) LPC_GPIO->DATA[DATAREG] = (LPC_GPIO->DATA[DATAREG] & ~(7<<7)) | (x << 7)

#define WHITE   0
#define CYAN    1
#define MAGENTA 2
#define BLUE    3
#define YELLOW  4
#define GREEN   5
#define RED     6
#define BLACK   7

#define LPC_GPIO0 ((LPC_GPIO_T *) LPC_GPIO_PORT0_BASE)
#define LPC_GPIO1 ((LPC_GPIO_T *) LPC_GPIO_PORT1_BASE)
#define LPC_GPIO2 ((LPC_GPIO_T *) LPC_GPIO_PORT2_BASE)
#define LPC_GPIO3 ((LPC_GPIO_T *) LPC_GPIO_PORT3_BASE)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#endif /* HELLO_H */
