/* settings.c */

#include "hello.h"
#include "settings.h"
#include "lfs.h"
#include "fs.h"

struct settings {
    uint8_t beep;
    uint8_t battery;
    uint8_t brightness;
    int default_file;
} settings;

bool settings_beep(void)
{
    return settings.beep;
}

int settings_brightness(void)
{
    return settings.brightness;
}

int settings_battery(void)
{
    return settings.battery;
}

int settings_default_file(void)
{
    return settings.default_file;
}

void settings_change_beep(void)
{
    settings.beep = !settings.beep;
}

void settings_set_default_file(int file)
{
    settings.default_file = file;
}

#define BRIGHTNESS_STEP 0x10
#define BRIGHTNESS_LAST_STEP (((255 / BRIGHTNESS_STEP) * BRIGHTNESS_STEP) + BRIGHTNESS_OFFSET)
#define BRIGHTNESS_OFFSET 1

void settings_change_brightness_up(void)
{
    if (settings.brightness == 255)
        settings.brightness = BRIGHTNESS_OFFSET;
    else if (settings.brightness >= BRIGHTNESS_LAST_STEP)
        settings.brightness = 255;
    else
        settings.brightness += BRIGHTNESS_STEP;
}

void settings_change_brightness_down(void)
{
    if (settings.brightness <= BRIGHTNESS_OFFSET)
        settings.brightness = 255;
    else if (settings.brightness > BRIGHTNESS_LAST_STEP)
        settings.brightness = BRIGHTNESS_LAST_STEP;
    else if (settings.brightness < BRIGHTNESS_STEP + BRIGHTNESS_OFFSET)
        settings.brightness = BRIGHTNESS_OFFSET;
    else
        settings.brightness -= BRIGHTNESS_STEP;
}

void settings_change_battery(void)
{
    if (settings.battery >= BATTERY_MAX)
        settings.battery = 0;
    else
        settings.battery++;
}

void settings_save(void)
{
    int ret = lfs_file_open(&fs_lfs, &fs_file, ".settings", LFS_O_WRONLY | LFS_O_CREAT);
    if (ret)
        return;

    ret = lfs_file_write(&fs_lfs, &fs_file, &settings, sizeof(settings));

    lfs_file_close(&fs_lfs, &fs_file);
}

void settings_load(void)
{
    settings.beep = true;
    settings.brightness = 0x7f;
    settings.battery = BATTERY_ON;
    settings.default_file = 0;

    int ret = lfs_file_open(&fs_lfs, &fs_file, ".settings", LFS_O_RDONLY);
    if (ret)
        return;

    ret = lfs_file_read(&fs_lfs, &fs_file, &settings, sizeof(settings));

    lfs_file_close(&fs_lfs, &fs_file);
}
