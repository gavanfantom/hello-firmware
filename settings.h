/* settings.h */

#ifndef SETTINGS_H
#define SETTINGS_H

#define BATTERY_ON      0
#define BATTERY_WHENLOW 1
#define BATTERY_OFF     2
#define BATTERY_MAX     2

bool settings_beep(void);
int settings_brightness(void);
int settings_battery(void);
void settings_change_beep(void);
void settings_change_brightness_up(void);
void settings_change_brightness_down(void);
void settings_change_battery(void);
void settings_save(void);
void settings_load(void);

#endif /* SETTINGS_H */
