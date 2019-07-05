/* i2c.h */

#ifndef I2C_H
#define I2C_H

#define I2C_SUCCESS             0
#define I2C_BUSY                1
#define I2C_FAIL_BUS_ERROR      2
#define I2C_FAIL_NACK           3
#define I2C_FAIL_ARBITRATION    4
#define I2C_FAIL_NOTIMPLEMENTED 5

void i2c_init(void);
bool i2c_busy(void);
void i2c_transmit(uint8_t address, uint8_t header, uint8_t *data, int len);
int i2c_result(void);

#endif /* I2C_H */
