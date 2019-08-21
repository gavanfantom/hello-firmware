/* battery.h */

#ifndef BATTERY_H
#define BATTERY_H

void battery_init(void);
void battery_poll(void);
void battery_timer(void);
int battery_status(void);

#define BATTERY_VREF 3300
#define BATTERY_FULLSCALE (BATTERY_VREF * 2)
#define BATTERY_RANGE 0xffc0
#define BATTERY_VALUE(x) ((x) * BATTERY_RANGE / BATTERY_FULLSCALE)

#define BATTERY_EMPTY      BATTERY_VALUE(3000)
#define BATTERY_THRESHOLD  BATTERY_VALUE(3300)
#define BATTERY_FULL       BATTERY_VALUE(4100)

#endif /* BATTERY_H */
