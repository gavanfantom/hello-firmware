/* spi.h */

#ifndef SPI_H
#define SPI_H

void spi_init(void);
void spi_transfer(uint8_t *out, uint8_t *in, int len);
void spi_start(void);
void spi_stop(void);

#define spi_write(data, len) spi_transfer(data, NULL, len)
#define spi_read(data, len)  spi_transfer(NULL, data, len)

#endif /* SPI_H */
