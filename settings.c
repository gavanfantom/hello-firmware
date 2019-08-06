/* settings.c */

#include "hello.h"
#include "settings.h"

struct settings {
    uint8_t beep;
    uint8_t battery;
    uint8_t brightness;
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

void settings_change_beep(void)
{
    settings.beep = !settings.beep;
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
    /* XXX write settings file */
}

void settings_load(void)
{
    /* XXX check for settings file */
    settings.beep = true;
    settings.brightness = 0x7f;
    settings.battery = BATTERY_ON;
}
