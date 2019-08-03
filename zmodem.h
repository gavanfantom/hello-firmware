/* zmodem.h */

#ifndef ZMODEM_H
#define ZMODEM_H

#define ZMODEM_BUFFER_SIZE 1024
#define ZMODEM_TIMEOUT 10 /* seconds */

void *zmodem_init(lfs_t *lfs, lfs_file_t *lfs_file);
void zmodem_reinit(void);
void zrx_byte(uint8_t byte);
void zmodem_timeout(void);
int zmodem_progress(void);
int zmodem_debug(void);
void zmodem_setactive(void);
bool zmodem_active(void);
bool zmodem_waiting(void);

#endif /* ZMODEM_H */
