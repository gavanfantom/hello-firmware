/* i2c.c */

#include "hello.h"
#include "i2c.h"

//#define I2C_FASTMODEPLUS
#define I2C_EXTRA

#ifdef I2C_EXTRA
# define I2CPINCONFIG (IOCON_FUNC1 | IOCON_FASTI2C_EN)
# define I2C_SPEED 2600000
# define SCL_LOWDUTY 75
#elif defined(I2C_FASTMODEPLUS)
# define I2CPINCONFIG (IOCON_FUNC1 | IOCON_FASTI2C_EN)
# define I2C_SPEED 1000000
#elif defined(I2C_FAST)
# define I2CPINCONFIG (IOCON_FUNC1 | IOCON_STDI2C_EN)
# define I2C_SPEED 400000
#else
# define I2CPINCONFIG (IOCON_FUNC1 | IOCON_STDI2C_EN)
# define I2C_SPEED 100000
#endif

#define I2C_PCLK 48000000
#define SCL_PERIOD (I2C_PCLK / I2C_SPEED)
#ifndef SCL_LOWDUTY
# define SCL_LOWDUTY 50
#endif
#define SCL_HIGHDUTY (100 - (SCL_LOWDUTY))
#define SCLL_VALUE (SCL_LOWDUTY  * SCL_PERIOD / 100)
#define SCLH_VALUE (SCL_HIGHDUTY * SCL_PERIOD / 100)

void i2c_init(void)
{
    LPC_IOCON->REG[IOCON_PIO0_4] = I2CPINCONFIG;
    LPC_IOCON->REG[IOCON_PIO0_5] = I2CPINCONFIG;
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_I2C);
    Chip_SYSCTL_DeassertPeriphReset(RESET_I2C0);
    LPC_I2C->CONSET = I2C_I2CONSET_I2EN;
    LPC_I2C->SCLH = SCLH_VALUE;
    LPC_I2C->SCLL = SCLL_VALUE;
    NVIC_EnableIRQ(I2C0_IRQn);
}


volatile struct {
    uint8_t *data;
    int len;
    int header;
    int offset;
    int address;
    volatile int result;
} i2c_state;

#define I2C_SUCCESS             0
#define I2C_BUSY                1
#define I2C_FAIL_BUS_ERROR      2
#define I2C_FAIL_NACK           3
#define I2C_FAIL_ARBITRATION    4
#define I2C_FAIL_NOTIMPLEMENTED 5

bool i2c_busy(void)
{
    return i2c_state.result == I2C_BUSY;
}

void i2c_transmit(uint8_t address, uint8_t header, uint8_t *data, int len)
{
    while (i2c_busy())
        ;

    i2c_state.address = address << 1;
    i2c_state.header = header;
    i2c_state.data = data;
    i2c_state.len = len;
    i2c_state.offset = -1;
    i2c_state.result = I2C_BUSY;
    LPC_I2C->CONSET = I2C_I2CONSET_STA;
}

int i2c_result(void)
{
    return i2c_state.result;
}

void I2C0_IRQHandler(void)
{
    int state = LPC_I2C->STAT;
    switch (state) {
    case 0x00: // Bus error
        LPC_I2C->CONSET = I2C_I2CONSET_STO | I2C_I2CONSET_AA;
        LPC_I2C->CONCLR = I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC;
        i2c_state.result = I2C_FAIL_BUS_ERROR;
        break;
    case 0x08: // START has been transmitted
    case 0x10: // Repeated START has been transmitted
        LPC_I2C->DAT = i2c_state.address;
        LPC_I2C->CONSET = I2C_I2CONSET_AA;
        LPC_I2C->CONCLR = I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC;
        break;
    case 0x18: // SLA+W has been transmitted, ACK has been received
    case 0x28: // Data has been transmitted, ACK has been received
        if (i2c_state.offset < 0) {
            LPC_I2C->DAT = i2c_state.header;
            i2c_state.offset++;
            LPC_I2C->CONSET = I2C_I2CONSET_AA;
            LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
        } else if (i2c_state.offset < i2c_state.len) {
            LPC_I2C->DAT = i2c_state.data[i2c_state.offset++];
            LPC_I2C->CONSET = I2C_I2CONSET_AA;
            LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
        } else {
            LPC_I2C->CONSET = I2C_I2CONSET_STO | I2C_I2CONSET_AA;
            LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
            i2c_state.result = I2C_SUCCESS;
        }
        break;
    case 0x20: // SLA+W has been transmitted, NACK has been received
    case 0x30: // Data has been transmitted, NACK has been received
        LPC_I2C->CONSET = I2C_I2CONSET_STO | I2C_I2CONSET_AA;
        LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
        i2c_state.result = I2C_FAIL_NACK;
        break;
    case 0x38: // Arbitration lost
        LPC_I2C->CONSET = I2C_I2CONSET_STO | I2C_I2CONSET_AA;
        LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
        i2c_state.result = I2C_FAIL_ARBITRATION;
        break;
    default:
        LPC_I2C->CONSET = I2C_I2CONSET_STO | I2C_I2CONSET_AA;
        LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
        i2c_state.result = I2C_FAIL_NOTIMPLEMENTED;
        break;
    }
  //  LPC_I2C->CONCLR = I2C_I2CONCLR_SIC;
}

