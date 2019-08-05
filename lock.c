/* lock.c */

#include "hello.h"
#include "uart.h"
#include "file.h"

bool file_locked;
bool file_locked_request;

void lock_init(void)
{
    file_locked = false;
    file_locked_request = false;
}

void check_file_lock(void)
{
    if (file_locked_request) {
        if (file_open) {
            stop_display();
        }
        file_locked = true;
        file_locked_request = false;
        uart_resume();
    }
    if (!file_locked && !file_open) {
        start_display();
    }
}

/* These functions may be called regardless of whether the lock is held */
void unlock_file(void)
{
    file_locked = false;
}

void lock_file(void)
{
    if (file_locked)
        return;
    file_locked_request = true;
    uart_pause();
}

