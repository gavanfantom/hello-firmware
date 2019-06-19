/* system.c */

#include "chip.h"

const uint32_t OscRateIn = 12000000;

void SystemInit(void)
{
    Chip_SystemInit();
}
