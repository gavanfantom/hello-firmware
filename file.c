/* file.c */

#include "hello.h"
#include "file.h"
#include "lfs.h"
#include "fs.h"
#include "frame.h"
#include <string.h>

#define FIRST_FILE 2

bool file_open;
struct filedata filedata;
int dir_offset;
bool safe_start;

void file_init(bool safe)
{
    dir_offset = FIRST_FILE;
    file_open = false;
    safe_start = safe;
    start_display();
}

void stop_display(void)
{
    displaytype = DISPLAY_NOTHING;
    if (file_open)
        lfs_file_close(&fs_lfs, &fs_file);
    file_open = 0;
    set_frame_rate(IDLE_FRAME_RATE);
}

void image_load(void)
{
    if (filedata.size < 8 + sizeof(filedata.image.header))
        goto fail;

    int ret = lfs_file_read(&fs_lfs, &fs_file, &filedata.image.header, sizeof(filedata.image.header));
    if (ret < 0)
        goto fail;

    if (filedata.image.header.height != 64)
        goto fail;

    if (filedata.image.header.width * 8 + filedata.image.header.data > filedata.size)
        goto fail;

    filedata.image.state.pos = filedata.image.header.start;
    set_frame_rate(filedata.image.header.frame_rate);

    displaytype = DISPLAY_IMAGE;

    return;

    fail:
        stop_display();
        return;
}

void video_load(void)
{
    if (filedata.size < 8 + sizeof(filedata.video.header))
        goto fail;

    int ret = lfs_file_read(&fs_lfs, &fs_file, &filedata.video.header, sizeof(filedata.video.header));
    if (ret < 0)
        goto fail;

    if (filedata.video.header.width != 128)
        goto fail;

    if (filedata.video.header.height != 64)
        goto fail;

    if (filedata.video.header.start < 0)
        goto fail;

    if (filedata.video.header.start >= filedata.video.header.frames)
        goto fail;

    if (filedata.video.header.frames * 1024 + filedata.video.header.data > filedata.size)
        goto fail;

    ret = lfs_file_seek(&fs_lfs, &fs_file, filedata.video.header.data + filedata.video.header.start * 1024, LFS_SEEK_SET);
    if (ret < 0)
        goto fail;

    filedata.video.state.started = true;
    filedata.video.state.frame = filedata.video.header.start;
    set_frame_rate(filedata.video.header.frame_rate);

    displaytype = DISPLAY_VIDEO;

    return;

    fail:
        stop_display();
        return;
}

void file_load(const char *filename)
{
    char magic[8];

    int ret = lfs_file_open(&fs_lfs, &fs_file, filename, LFS_O_RDONLY);
    if (ret)
        return;

    displaytype = DISPLAY_NOTHING;
    file_open = true;

    ret = lfs_file_size(&fs_lfs, &fs_file);
    if (ret < 0) {
        stop_display();
        return;
    }

    filedata.size = ret;

    ret = lfs_file_read(&fs_lfs, &fs_file, magic, 8);
    if (ret < 0) {
        stop_display();
        return;
    }

    if (memcmp(magic, "helloimg", 8) == 0)
        image_load();
    else if (memcmp(magic, "hellovid", 8) == 0)
        video_load();
    else
        stop_display();
}

int load_file_by_offset(int target)
{
    lfs_dir_t dir;
    if (safe_start)
        return 0;
    if (file_open)
        stop_display();
    int err = lfs_dir_open(&fs_lfs, &dir, "/");
    if (err) {
        return -1;
    }
    struct lfs_info info;
    int last_fileno = 0;
    int fileno;
    for (fileno = 0; true; fileno++) {
        int res  = lfs_dir_read(&fs_lfs, &dir, &info);
        if (res < 0) {
            lfs_dir_close(&fs_lfs, &dir);
            return res;
        }
        if (res == 0) {
            if (target == FIRST_FILE) {
                lfs_dir_close(&fs_lfs, &dir);
                return 0;
            }
            fileno = -1; // This will be incremented on continue
            if (target < 0) {
                target = last_fileno;
            } else {
                target = FIRST_FILE;
            }
            lfs_dir_rewind(&fs_lfs, &dir);
            continue;
        }

        if (fileno < target)
            continue;

        last_fileno = fileno;

        if (fileno == target) {
            if (info.type != LFS_TYPE_REG) {
                target++;
                continue;
            }

            lfs_dir_close(&fs_lfs, &dir);
            file_load(info.name);
            return fileno;
        }
    }
}

void next_file(void)
{
    dir_offset = load_file_by_offset(dir_offset + 1);
}

void prev_file(void)
{
    dir_offset--;
    if (dir_offset < FIRST_FILE)
        dir_offset = -1;
    int res = load_file_by_offset(dir_offset);
    if (dir_offset < FIRST_FILE)
        dir_offset = res;
}

void start_display(void)
{
    dir_offset = load_file_by_offset(dir_offset);
}
