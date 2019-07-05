/* speaker.c */

#include "hello.h"
#include "speaker.h"

#define TIMER_PCLK 48000000
void speaker_init(void)
{
    /*
     * PIO2_6 / CT32B0_MAT1
     * PIO2_7 / CT32B0_MAT2
     */

    LPC_IOCON->REG[IOCON_PIO2_6] = IOCON_FUNC1;
    LPC_IOCON->REG[IOCON_PIO2_7] = IOCON_FUNC1;
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CT32B0);
    Chip_SYSCTL_DeassertPeriphReset(RESET_TIMER0_32);
    LPC_TIMER32_0->TCR = 2; // CRst
    LPC_TIMER32_0->TCR = 0; // CRst
    LPC_TIMER32_0->PR = 0;  // No prescaling - yet!
    LPC_TIMER32_0->MCR = 2; // MR0R
    LPC_TIMER32_0->MR[1] = 0; // Match start of cycle
    LPC_TIMER32_0->MR[2] = 0; // Match start of cycle
    LPC_TIMER32_0->EMR = 0x000003c2; // EMC1=EMC2=3=Toggle on match
    LPC_TIMER32_0->CTCR = 0; // Timer mode
    LPC_TIMER32_0->PWMC = 0; // Not PWM mode
}


void speaker_on(int frequency)
{
    LPC_IOCON->REG[IOCON_PIO2_6] = IOCON_FUNC1;
    LPC_IOCON->REG[IOCON_PIO2_7] = IOCON_FUNC1;
    LPC_TIMER32_0->MR[0] = TIMER_PCLK / frequency;
    LPC_TIMER32_0->TC = 0;
    LPC_TIMER32_0->TCR = 1; // CEn
}

void speaker_off(void)
{
    LPC_IOCON->REG[IOCON_PIO2_6] = 0;
    LPC_IOCON->REG[IOCON_PIO2_7] = 0;
    LPC_TIMER32_0->TCR = 0; // Off
}

void beep_up(void)
{
    speaker_on(1000);
    for (volatile int i = 0; i < 0x10000; i++)
        ;
    speaker_off();

    speaker_on(1500);
    for (volatile int i = 0; i < 0x10000; i++)
        ;
    speaker_off();
}

void beep_down(void)
{
    speaker_on(1500);
    for (volatile int i = 0; i < 0x10000; i++)
        ;
    speaker_off();

    speaker_on(1000);
    for (volatile int i = 0; i < 0x10000; i++)
        ;
    speaker_off();
}

