/* fs.h */

#ifndef FS_H
#define FS_H

/* Autodetect block_count if we ever want to use a different flash chip */
#define BLOCK_SIZE 4096
#define BLOCK_COUNT 2048

extern lfs_t                   fs_lfs;
extern lfs_file_t              fs_file;
extern const struct lfs_config fs_cfg;

void fs_init(void);

#endif /* FS_H */
