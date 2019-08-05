/* speaker.c */

#include "hello.h"
#include "timer.h"
#include "speaker.h"

void speaker_init(void)
{
    /*
     * PIO2_6 / CT32B0_MAT1
     * PIO2_7 / CT32B0_MAT2
     */

    LPC_IOCON->REG[IOCON_PIO2_6] = IOCON_FUNC1;
    LPC_IOCON->REG[IOCON_PIO2_7] = IOCON_FUNC1;
    Chip_Clock_EnablePeriphClock(SPEAKER_CLOCK);
    Chip_SYSCTL_DeassertPeriphReset(SPEAKER_RESET);
    SPEAKER_BASE->TCR = 2; // CRst
    SPEAKER_BASE->TCR = 0; // CRst
    SPEAKER_BASE->PR = SPEAKER_PRESCALE;
    SPEAKER_BASE->MCR = 2; // MR0R
    SPEAKER_BASE->MR[1] = 0; // Match start of cycle
    SPEAKER_BASE->MR[2] = 0; // Match start of cycle
    SPEAKER_BASE->EMR = 0x000003c2; // EMC1=EMC2=3=Toggle on match
    SPEAKER_BASE->CTCR = 0; // Timer mode
    SPEAKER_BASE->PWMC = 0; // Not PWM mode
}


void speaker_on(int frequency)
{
    LPC_IOCON->REG[IOCON_PIO2_6] = IOCON_FUNC1;
    LPC_IOCON->REG[IOCON_PIO2_7] = IOCON_FUNC1;
    SPEAKER_BASE->MR[0] = TIMER_PCLK / (frequency * 2 * SPEAKER_PRESCALE);
    SPEAKER_BASE->TC = 0;
    SPEAKER_BASE->TCR = 1; // CEn
}

void speaker_off(void)
{
    LPC_IOCON->REG[IOCON_PIO2_6] = 0;
    LPC_IOCON->REG[IOCON_PIO2_7] = 0;
    SPEAKER_BASE->TCR = 0; // Off
}

void beep(int frequency, int duration)
{
    speaker_on(frequency);
    delay(duration);
    speaker_off();
}

void beep_up(void)
{
    beep(500, 20);
    beep(750, 20);
}

void beep_down(void)
{
    beep(750, 20);
    beep(500, 20);
}

