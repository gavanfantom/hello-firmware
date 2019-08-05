/* fs.c */

#include "hello.h"
#include "spi.h"
#include "spiflash.h"
#include "lfs.h"
#include "fs.h"

int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    int address = c->block_size * block + off;
    spiflash_read(address, buffer, size);
    return LFS_ERR_OK;
}

int flash_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int address = c->block_size * block + off;
    if (spiflash_write(address, (void *)buffer, size))
        return LFS_ERR_OK;
    return LFS_ERR_IO;
}

int flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    int address = c->block_size * block;
    if (spiflash_erase_sector(address))
        return LFS_ERR_OK;
    return LFS_ERR_IO;
}

int flash_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK;
}

lfs_t      fs_lfs;
lfs_file_t fs_file;

uint8_t read_buffer[16];
uint8_t prog_buffer[16];
uint8_t lookahead_buffer[16] __attribute__ ((aligned (8)));

const struct lfs_config fs_cfg = {
    .read = flash_read,
    .prog = flash_write,
    .erase = flash_erase,
    .sync = flash_sync,

    .read_size = 1,
    .prog_size = 1,
    .block_size = BLOCK_SIZE,
    .block_count = BLOCK_COUNT,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 1000,
    .read_buffer = read_buffer,
    .prog_buffer = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};

void fs_init(void)
{
    spi_init();
    spiflash_reset();
    spiflash_address_mode3();

    int err = lfs_mount(&fs_lfs, &fs_cfg);
    if (err) {
        lfs_format(&fs_lfs, &fs_cfg);
        lfs_mount(&fs_lfs, &fs_cfg);
    }
}
