/* zmodem.h */

#ifndef ZMODEM_H
#define ZMODEM_H

void zmodem_init(lfs_t *lfs, lfs_file_t *lfs_file);
void zmodem_reinit(void);
void zrx_byte(uint8_t byte);
void zmodem_timeout(void);
int zmodem_progress(void);
int zmodem_debug(void);
bool zmodem_active(void);

#endif /* ZMODEM_H */
