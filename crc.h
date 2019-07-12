/* crc.h */

#ifndef CRC_H
#define CRC_H

uint16_t crc16(const void *data, int length, uint16_t crc);
uint32_t crc32(const void *data, int length, uint32_t crc);

#endif /* CRC_H */
