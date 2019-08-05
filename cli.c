/* cli.c */

#include <string.h>
#include "hello.h"
#include "lfs.h"
#include "cli.h"
#include "uart.h"
#include "zmodem.h"
#include "timer.h"
#include "uart.h"
#include "write.h"
#include "fs.h"
#include "lock.h"

#define CMD_BUFFER_SIZE ZMODEM_BUFFER_SIZE

#define STATE_CLI 0
#define STATE_ZMODEM 1

int uart_state;
char *cmd_buffer;
int cmd_ptr = 0;
int arg_ptr = 0;

void cli_reinit(void)
{
    cmd_ptr = 0;
    arg_ptr = 0;
    uart_state = STATE_CLI;
}

void cli_init(void)
{
    cli_reinit();
    cmd_buffer = zmodem_init(&fs_lfs, &fs_file);
    timeout_timer_init();
}

void check_zmodem(void)
{
    if (!zmodem_active()) {
        uart_state = STATE_CLI;
        timeout_timer_off();
        unlock_file();
    }
}

void TIMEOUT_HANDLER(void)
{
    TIMEOUT_BASE->IR = 15;
    zmodem_timeout();
    check_zmodem();
}

void cmd_rz(void)
{
    lock_file();
    zmodem_setactive();
    uart_state = STATE_ZMODEM;
    timeout_timer_on(ZMODEM_TIMEOUT);
}

void cmd_ls(void)
{
    lfs_dir_t dir;
    int err = lfs_dir_open(&fs_lfs, &dir, "/");
    if (err) {
        uart_write_string("Error opening directory\r\n");
        return;
    }
    struct lfs_info info;
    while (true) {
        int res  = lfs_dir_read(&fs_lfs, &dir, &info);
        if (res < 0) {
            uart_write_string("Error reading directory\r\n");
            lfs_dir_close(&fs_lfs, &dir);
        }
        if (res == 0)
            break;
        switch (info.type) {
        case LFS_TYPE_REG:
            uart_write_string("f ");
            break;
        case LFS_TYPE_DIR:
            uart_write_string("d ");
            break;
        default:
            uart_write_string("? ");
            break;
        }
        int len = 1;
        for (int size = info.size; size >= 10; size /= 10)
            len++;
        for (int i = 10-len; i; i--)
            uart_write_string(" ");
        uart_write_int(info.size);
        uart_write_string(" ");
        uart_write_string(info.name);
        uart_write_string("\r\n");
    }
    lfs_dir_close(&fs_lfs, &dir);
}

const char *lfs_error(int error)
{
    switch (error) {
    case LFS_ERR_IO:          return "I/O error";
    case LFS_ERR_CORRUPT:     return "Corrupted";
    case LFS_ERR_NOENT:       return "File not found";
    case LFS_ERR_EXIST:       return "Entry already exists";
    case LFS_ERR_NOTDIR:      return "Not a directory";
    case LFS_ERR_ISDIR:       return "Is a directory";
    case LFS_ERR_NOTEMPTY:    return "Directory not empty";
    case LFS_ERR_BADF:        return "Bad file number";
    case LFS_ERR_FBIG:        return "File too big";
    case LFS_ERR_INVAL:       return "Invalid argument";
    case LFS_ERR_NOSPC:       return "No space left on device";
    case LFS_ERR_NOMEM:       return "Out of memory";
    case LFS_ERR_NOATTR:      return "No attribute";
    case LFS_ERR_NAMETOOLONG: return "File name too long";
    }
    return "Unknown error";
}

void cmd_rm(void)
{
    if (arg_ptr == 0) {
        uart_write_string("Usage: rm <filename>\r\n");
        return ;
    }

    char *filename = cmd_buffer + arg_ptr;

    int err = lfs_remove(&fs_lfs, filename);
    if (err) {
        uart_write_string(lfs_error(err));
        uart_write_string("\r\n");
        return;
    }
}

void cmd_free(void)
{
    lfs_ssize_t res = lfs_fs_size(&fs_lfs);
    uart_write_int(res);
    uart_write_string("/");
    uart_write_int(BLOCK_COUNT);
    uart_write_string(" blocks used (");
    uart_write_int(100*res/BLOCK_COUNT);
    uart_write_string("%)\r\n");
}

void cmd_format(void)
{
    uart_write_string("Formatting filesystem... ");
    lfs_unmount(&fs_lfs);
    lfs_format(&fs_lfs, &fs_cfg);
    int err = lfs_mount(&fs_lfs, &fs_cfg);
    if (err < 0)
        uart_write_string("failed\r\n");
    else
        uart_write_string("done\r\n");
}

void cmd_reset(void)
{
    NVIC_SystemReset();
}

typedef struct {
    const char *command;
    void (*fn)(void);
} command;

void cmd_help(void);

static command cmd_table[] = {
    {"help", &cmd_help},
    {"rz", &cmd_rz},
    {"ls", &cmd_ls},
    {"rm", &cmd_rm},
    {"free", &cmd_free},
    {"format", &cmd_format},
    {"reset", &cmd_reset},
};

void process_command(void)
{
    cmd_buffer[cmd_ptr] = '\0';

    cmd_ptr = 0;

    char *cmd = cmd_buffer + strspn(cmd_buffer, " ");
    char *delim = strchr(cmd, ' ');
    if (delim) {
        char *arg = delim + strspn(delim, " ");
        arg_ptr = arg - cmd_buffer;
        *delim = '\0';
    } else {
        arg_ptr = 0;
    }

    for (int i = 0; i < ARRAY_SIZE(cmd_table); i++) {
        if (strcmp(cmd, cmd_table[i].command) == 0) {
            cmd_table[i].fn();
            return;
        }
    }
    uart_write_string("Unrecognised command\r\n");
}

void cmd_help(void)
{
    for (int i = 0; i < ARRAY_SIZE(cmd_table); i++) {
        uart_write_string(cmd_table[i].command);
        uart_write_string("\r\n");
    }
}

void uart_rxhandler(uint8_t byte)
{
    switch (uart_state) {
    case STATE_CLI:
        switch (byte) {
        case 0:
            break;
        case 3: // Ctrl-C
            cmd_ptr = 0;
            uart_write_string("\r\n");
            break;
        case '\b':
            if (cmd_ptr) {
                cmd_ptr--;
                uart_transmit(byte);
            }
            break;
        case 21: // Ctrl-U
            cmd_ptr = 0;
            uart_write_string("\r\e[K");
            break;
        case '\r':
        case '\n':
            uart_write_string("\r\n");
            process_command();
            break;
        default:
            if (cmd_ptr < CMD_BUFFER_SIZE-1) {
                cmd_buffer[cmd_ptr++] = byte;
                uart_transmit(byte);
            }
            break;
        }
        if (cmd_ptr) {
            lock_file();
        } else {
            if (uart_state == STATE_CLI)
                unlock_file();
        }
        break;
    case STATE_ZMODEM:
        timeout_timer_reset();
        zrx_byte(byte);
        check_zmodem();
        break;
    }
}

