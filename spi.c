/* spi.c */

#include "hello.h"
#include "spi.h"

#define SPI0_PCLK 48000000
void spi_init(void)
{
    LPC_IOCON->REG[IOCON_PIO0_2] = 0; // CS (FUNC1 is SSEL0, 0 is GPIO)
    LPC_IOCON->REG[IOCON_PIO0_3] = IOCON_MODE_PULLDOWN; // WP
    LPC_IOCON->REG[IOCON_PIO0_6] = IOCON_FUNC2; // SCK
    LPC_IOCON->REG[IOCON_PIO0_8] = IOCON_FUNC1; // MISO
    LPC_IOCON->REG[IOCON_PIO0_9] = IOCON_FUNC1; // MOSI
    LPC_IOCON->REG[IOCON_PIO0_11] = IOCON_FUNC1; // RESET (FUNC1 is GPIO)
    LPC_GPIO->DIR               |= (1<<11) | (1<<3) | (1<<2); // outputs
    LPC_GPIO->DATA[DATAREG] = LPC_GPIO->DATA[DATAREG] | (1<<11) | (1 << 3) | (1 << 2);
    Chip_IOCON_PinLocSel(LPC_IOCON, IOCON_SCKLOC_PIO0_6); // PIO0_6
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SSP0);
    LPC_SYSCTL->SSP0CLKDIV = 1;
    Chip_SYSCTL_DeassertPeriphReset(RESET_SSP0);
    LPC_SSP0->CR0 = SSP_CR0_DSS(7) | SSP_CR0_FRF_SPI | SSP_CR0_CPOL_LO | SSP_CR0_CPHA_FIRST | SSP_CR0_SCR(0);
    LPC_SSP0->CPSR = 2; // Yes, 2 is the minimum.
    LPC_SSP0->CR1 = SSP_CR1_SSP_EN;
}

void spi_transfer(uint8_t *out, uint8_t *in, int len)
{
    int i = 0;
    int j = 0;
    while ((i < len) || (j < len)) {
        if ((i < len) && (LPC_SSP0->SR & SSP_STAT_TNF)) {
            LPC_SSP0->DR = out?(out[i]):0;
            i++;
        }
        if ((j < len) && (LPC_SSP0->SR & SSP_STAT_RNE)) {
            uint8_t byte = LPC_SSP0->DR;
            if (in)
                in[j] = byte;
            j++;
        }
    }
}

#define spi_write(data, len) spi_transfer(data, NULL, len)
#define spi_read(data, len)  spi_transfer(NULL, data, len)

void spi_start(void)
{
    LPC_GPIO->DATA[DATAREG] = LPC_GPIO->DATA[DATAREG] & ~(1<<2);
}

void spi_stop(void)
{
    LPC_GPIO->DATA[DATAREG] = LPC_GPIO->DATA[DATAREG] | (1 << 2);
}

