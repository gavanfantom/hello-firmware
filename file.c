/* file.c */

#include "hello.h"
#include "file.h"
#include "lfs.h"
#include "fs.h"
#include "frame.h"
#include <string.h>
#include "menu.h"
#include "settings.h"

bool file_open;
struct filedata filedata;
int dir_offset;
bool safe_start;

void file_init(bool safe)
{
    dir_offset = settings_default_file();
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
    menu = MENU_NONE;
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

char *file_read_dirent(lfs_dir_t *dir, struct lfs_info *info, bool ignore_dot)
{
    while (1) {
        int res = lfs_dir_read(&fs_lfs, dir, info);
        if (res != 1)
            return NULL;
        if (info->type != LFS_TYPE_REG)
            continue;
        if (ignore_dot && (info->name[0] == '.'))
            continue;
        return info->name;
    }
}

char *file_find(lfs_dir_t *dir, struct lfs_info *info, int fileno, bool ignore_dot)
{
    char *name = NULL;
    if (lfs_dir_open(&fs_lfs, dir, "/"))
        return NULL;
    for (int i = 0; i <= fileno; i++) {
        name = file_read_dirent(dir, info, ignore_dot);
        if (!name)
            break;
    }
    lfs_dir_close(&fs_lfs, dir);
    return name;
}

int file_count(bool ignore_dot)
{
    lfs_dir_t dir;
    struct lfs_info info;
    int count = 0;
    if (lfs_dir_open(&fs_lfs, &dir, "/"))
        return 0;
    while (file_read_dirent(&dir, &info, ignore_dot))
        count++;
    lfs_dir_close(&fs_lfs, &dir);
    return count;
}

int load_file_by_offset(int target)
{
    lfs_dir_t dir;
    struct lfs_info info;
    char *name = file_find(&dir, &info, target, true);
    if (name) {
        file_load(name);
        return target;
    }
    if (target < 0)
        target = file_count(true) - 1;
    else
        target = 0;
    name = file_find(&dir, &info, target, true);
    if (name) {
        file_load(name);
        return target;
    }
    return 0;
}

void file_update_offset(const char *filename)
{
    lfs_dir_t dir;
    if (file_open)
        stop_display();
    if (lfs_dir_open(&fs_lfs, &dir, "/"))
        return;
    struct lfs_info info;
    int fileno;
    for (fileno = 0; true; fileno++) {
        char *name = file_read_dirent(&dir, &info, true);
        if (!name)
            break;

        if (strcmp(name, filename) == 0) {
            dir_offset = fileno;
            break;
        }
    }

    lfs_dir_close(&fs_lfs, &dir);
}

void file_load_update_offset(const char *filename)
{
    file_update_offset(filename);
    file_load(filename);
}

void file_set_default(const char *filename)
{
    file_update_offset(filename);
    settings_set_default_file(dir_offset);
}

void next_file(void)
{
    dir_offset = load_file_by_offset(dir_offset + 1);
}

void prev_file(void)
{
    dir_offset = load_file_by_offset(dir_offset - 1);
}

void start_display(void)
{
    dir_offset = load_file_by_offset(dir_offset);
}
