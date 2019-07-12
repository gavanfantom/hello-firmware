/* spiflash.h */

#ifndef SPIFLASH_H
#define SPIFLASH_H

void spiflash_reset(void);
void spiflash_wp_off(void);
void spiflash_wp_on(void);
void spiflash_read_deviceid(uint8_t *data, int len);
void spiflash_short_command(uint8_t command);

#define spiflash_address_mode3() spiflash_short_command(0xe9)
#define spiflash_address_mode4() spiflash_short_command(0xb7)
#define spiflash_clsr() spiflash_short_command(0x30)
#define spiflash_write_enable() spiflash_short_command(0x06)
#define spiflash_write_disable() spiflash_short_command(0x04)

uint8_t spiflash_status(uint8_t command, uint8_t mask);

#define spiflash_status1() spiflash_status(0x05, 0)
#define spiflash_status2() spiflash_status(0x07, 0)

#define spiflash_status1_poll(mask) spiflash_status(0x05, (mask))
#define spiflash_status2_poll(mask) spiflash_status(0x07, (mask))

void spiflash_read(uint32_t address, uint8_t *data, int len);
bool spiflash_write_page(uint32_t address, uint8_t *data, int len);

bool spiflash_write(uint32_t address, uint8_t *data, int len);
bool spiflash_erase_chip(void);
bool spiflash_erase_sector(uint32_t address);
bool spiflash_erase(uint32_t address, int len);

#endif /* SPIFLASH_H */
