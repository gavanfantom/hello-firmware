/* battery.c */

#include "hello.h"
#include "battery.h"
#include "timer.h"

void battery_read_enable(bool on);

#define ADC_CR (ADC_CR_CH_SEL(1) | ADC_CR_CLKDIV(11))
#define BATTERY_COUNT 2
#define BATTERY_INTERVAL 1000

int battery_counter;
int battery_reading;

void battery_init(void)
{
    battery_counter = BATTERY_COUNT;
    LPC_IOCON->REG[IOCON_PIO1_0] = IOCON_FUNC2;
    LPC_IOCON->REG[IOCON_PIO1_1] = IOCON_FUNC1 | IOCON_ADMODE_EN;
    LPC_GPIO1->DIR           |= (1<<1); // output
    battery_read_enable(false);
    LPC_SYSCTL->PDRUNCFG &= ~0x10; // Enable power to ADC
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_ADC);
    Chip_SYSCTL_DeassertPeriphReset(RESET_ADC0);
    LPC_ADC->CR = ADC_CR;
    LPC_ADC->INTEN = 1<<1;
    NVIC_EnableIRQ(ADC_IRQn);
}

void battery_read_enable(bool on)
{
    if (on)
        LPC_GPIO1->DATA[DATAREG] |= (1<<1);
    else
        LPC_GPIO1->DATA[DATAREG] &= ~(1<<1);
}

void ADC_IRQHandler(void)
{
    LPC_ADC->CR = ADC_CR;
    int data = LPC_ADC->DR[1];
    battery_reading = data & 0xffff;
    battery_read_enable(false);
}

void battery_read(void)
{
    LPC_ADC->CR = ADC_CR | ADC_CR_START_NOW;
}

void battery_poll(void)
{
    uint32_t time = systime_timer_get();
    static uint32_t last_time = 0;
    if ((time - last_time) >= BATTERY_INTERVAL) {
        last_time = time;
        battery_timer();
    }
    if (battery_counter) {
        if (--battery_counter == 0)
            battery_read();
    }
}

void battery_timer(void)
{
    battery_read_enable(true);
    battery_counter = BATTERY_COUNT;
}

int battery_status(void)
{
    return battery_reading;
}
