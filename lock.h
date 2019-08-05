/* lock.h */

#ifndef LOCK_H
#define LOCK_H

extern bool file_locked;

void lock_init(void);
void check_file_lock(void);
void unlock_file(void);
void lock_file(void);

#endif /* LOCK_H */
