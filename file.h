/* file.h */

#ifndef FILE_H
#define FILE_H

#include "lfs.h"

extern bool file_open;
extern bool safe_start;

#define ACTION_ONESHOT 0
#define ACTION_LOOP    1
#define ACTION_REVERSE 2

struct imageheader {
    int version;
    int data;
    int width;
    int height;
    int frame_rate;
    int scroll_rate;
    int action;
    int left_blank;
    int right_blank;
    int left_blank_pattern;
    int right_blank_pattern;
    int left_pause;
    int right_pause;
    int start;
};

struct imagestate {
    int pos;
};

struct videoheader {
    int version;
    int data;
    int width;
    int height;
    int frames;
    int frame_rate;
    int action;
    int start;
};

struct videostate {
    bool started;
    int frame;
};

struct filedata {
    int size;
    union {
        struct {
            struct imageheader header;
            struct imagestate state;
        } image;
        struct {
            struct videoheader header;
            struct videostate state;
        } video;
    };
};

extern struct filedata filedata;

void file_init(bool safe);
void start_display(void);
void stop_display(void);

void image_load(void);
void video_load(void);

void file_load(const char *filename);
void file_load_update_offset(const char *filename);

int load_file_by_offset(int target);
void next_file(void);
void prev_file(void);

char *file_read_dirent(lfs_dir_t *dir, struct lfs_info *info, bool ignore_dot);
char *file_find(lfs_dir_t *dir, struct lfs_info *info, int fileno, bool ignore_dot);
int file_count(bool ignore_dot);
void file_set_default(const char *filename);

#endif /* FILE_H */
