/* timer.c */

#include "hello.h"
#include "timer.h"

void frame_timer_init(void)
{
    Chip_Clock_EnablePeriphClock(FRAME_CLOCK);
    Chip_SYSCTL_DeassertPeriphReset(FRAME_RESET);
    FRAME_BASE->TCR = 2; // CRst
    FRAME_BASE->TCR = 0; // CRst
    FRAME_BASE->PR = FRAME_PRESCALE-1;
    FRAME_BASE->MCR = 3; // MR0I | MR0R
    FRAME_BASE->CTCR = 0; // Timer mode
    FRAME_BASE->PWMC = 0; // Not PWM mode
    NVIC_EnableIRQ(FRAME_IRQN);
}

void frame_timer_on(int frequency)
{
    uint32_t prescaler = 1;
    uint32_t divider = TIMER_PCLK / frequency;
    while (divider > FRAME_MAX) {
        prescaler = prescaler * 2;
        divider = divider / 2;
    }
    FRAME_BASE->TCR = 0; // Off
    FRAME_BASE->MR[0] = divider;
    FRAME_BASE->PR = prescaler - 1;
    FRAME_BASE->TC = 0;
    FRAME_BASE->TCR = 1; // CEn
}

void frame_timer_off(void)
{
    FRAME_BASE->TCR = 0; // Off
}

