/* spiflash.c */

#include "hello.h"
#include "spiflash.h"
#include "spi.h"
#include "write.h"
#include "timer.h"

void spiflash_reset(void)
{
    LPC_GPIO->DATA[DATAREG] = LPC_GPIO->DATA[DATAREG] & ~(1<<11);
    delay(1);
    LPC_GPIO->DATA[DATAREG] = LPC_GPIO->DATA[DATAREG] | (1<<11);
}

void spiflash_wp_off(void)
{
    LPC_GPIO->DATA[DATAREG] = LPC_GPIO->DATA[DATAREG] | (1<<3);
}

void spiflash_wp_on(void)
{
    LPC_GPIO->DATA[DATAREG] = LPC_GPIO->DATA[DATAREG] & ~(1<<3);
}

void spiflash_read_deviceid(uint8_t *data, int len)
{
    uint8_t command = 0x9f;
    spi_start();
    spi_transfer(&command, NULL, 1);
    spi_transfer(NULL, data, len);
    spi_stop();
}

void spiflash_short_command(uint8_t command)
{
    spi_start();
    spi_transfer(&command, NULL, 1);
    spi_stop();
}

#define spiflash_address_mode3() spiflash_short_command(0xe9)
#define spiflash_address_mode4() spiflash_short_command(0xb7)
#define spiflash_clsr() spiflash_short_command(0x30)
#define spiflash_write_enable() spiflash_short_command(0x06)
#define spiflash_write_disable() spiflash_short_command(0x04)


uint8_t spiflash_status(uint8_t command, uint8_t mask)
{
    uint8_t data;

    spi_start();
    spi_transfer(&command, NULL, 1);
    do {
        spi_transfer(NULL, &data, 1);
    } while (data & mask);
    spi_stop();

    return data;
}

#define spiflash_status1() spiflash_status(0x05, 0)
#define spiflash_status2() spiflash_status(0x07, 0)

#define spiflash_status1_poll(mask) spiflash_status(0x05, (mask))
#define spiflash_status2_poll(mask) spiflash_status(0x07, (mask))

void spiflash_read(uint32_t address, uint8_t *data, int len)
{
    uint32_t command = __builtin_bswap32(address) | 0x03;
//    uart_write_string("R: ");
//    uart_write_hex(command);
//    uart_write_string("\r\n");
    spi_start();
    spi_transfer((uint8_t *)&command, NULL, 4);
    spi_transfer(NULL, data, len);
    spi_stop();
//    uart_write_hex(((uint32_t *)data)[0]);
//    uart_write_string("\r\n");
}

bool spiflash_write_page(uint32_t address, uint8_t *data, int len)
{
    uint32_t command = __builtin_bswap32(address) | 0x02;
    spiflash_wp_off();
    spiflash_write_enable();
    spi_start();
    spi_transfer((uint8_t *)&command, NULL, 4);
    spi_transfer(data, NULL, len);
    spi_stop();
//    uart_write_hex(spiflash_status1() | (spiflash_status2() << 16));

    while ((spiflash_status1() & 0x01) && !(spiflash_status2() & 0x20))
        ;

    spiflash_wp_on();

    if (spiflash_status2() & 0x20) {
        spiflash_clsr();
        return false;
    }

    return true;
}

#define PAGE_SIZE 256
#define ERASE_SIZE 4096

#define PAGE(x) ((x) & ~(PAGE_SIZE-1))

bool spiflash_write(uint32_t address, uint8_t *data, int len)
{
    while (len) {
        uint32_t limit = PAGE(address) + PAGE_SIZE;
        int pagelen = limit - address;
        if (len < pagelen) 
            pagelen = len;
//        uart_write_hex(address);
//        uart_write_string(": ");
//        uart_write_hex(pagelen);
//        uart_write_string(": ");
//        uart_write_hex(((uint32_t *)data)[0]);
//        uart_write_string("\r\n");
        if (!spiflash_write_page(address, data, pagelen))
            return false;
        address += pagelen;
        data += pagelen;
        len -= pagelen;
    }

//    uart_write_string("Write complete\r\n");
    return true;
}

bool spiflash_erase_chip(void)
{
    spiflash_wp_off();
    spiflash_write_enable();
    spiflash_short_command(0x60);

    while ((spiflash_status1() & 0x01) && !(spiflash_status2() & 0x40))
        ;

    spiflash_wp_on();

    if (spiflash_status2() & 0x40) {
        spiflash_clsr();
        return false;
    }

    return true;
}

bool spiflash_erase_sector(uint32_t address)
{
    uint32_t command = __builtin_bswap32(address) | 0x20;
//    uart_write_string("E: ");
//    uart_write_hex(command);
//    uart_write_string("\r\n");
    spiflash_wp_off();
    spiflash_write_enable();
    spi_start();
    spi_transfer((uint8_t *)&command, NULL, 4);
    spi_stop();

    while ((spiflash_status1() & 0x01) && !(spiflash_status2() & 0x40))
        ;

    spiflash_wp_on();

    if (spiflash_status2() & 0x40) {
        spiflash_clsr();
        return false;
    }

    return true;
}

bool spiflash_erase(uint32_t address, int len)
{
    while (len > 0) {
        if (!spiflash_erase_sector(address))
            return false;
        address += ERASE_SIZE;
        len -= ERASE_SIZE;
    }
    return true;
}
