/* button.c */

#include "hello.h"
#include "button.h"
#include "timer.h"

#define BUTTON_UP     0x01
#define BUTTON_DOWN   0x02
#define BUTTON_LEFT   0x04
#define BUTTON_RIGHT  0x08
#define BUTTON_CENTRE 0x10

void button_init(void)
{
    LPC_IOCON->REG[IOCON_PIO3_0] = IOCON_MODE_PULLUP; // Up
    LPC_IOCON->REG[IOCON_PIO3_1] = IOCON_MODE_PULLUP; // Down
    LPC_IOCON->REG[IOCON_PIO3_2] = IOCON_MODE_PULLUP; // Left
    LPC_IOCON->REG[IOCON_PIO3_3] = IOCON_MODE_PULLUP; // Right
    LPC_IOCON->REG[IOCON_PIO3_4] = IOCON_MODE_PULLUP; // Centre
}

int button_state(void)
{
    return ~LPC_GPIO3->DATA[DATAREG] & 0x1f;
}

#define DEBOUNCE_TIME 10
#define FIRST_REPEAT_TIME 300
#define REPEAT_TIME 100

int button_event(void)
{
    static int buttons = 0;
    static uint32_t last_changed[5] = {0, 0, 0, 0, 0};
    static uint32_t last_event[5] = {0, 0, 0, 0, 0};
    int new_buttons = button_state();
    int changed = new_buttons ^ buttons;
    uint32_t now = systime_timer_get();

    for (int i = 0; i < 5; i++) {
        if (changed & (1<<i)) {
            if (now - last_changed[i] >= DEBOUNCE_TIME) {
                last_changed[i] = now;
                last_event[i] = now;
                buttons = (buttons & ~(1<<i)) | (new_buttons & (1<<i));
            }
        }
        if (buttons & (1<<i)) {
            int threshold = (last_changed[i] == last_event[i])?FIRST_REPEAT_TIME:REPEAT_TIME;
            if (now - last_event[i] >= threshold) {
                last_event[i] += threshold;
                changed |= (1<<i);
            }
        }
    }

    return buttons & changed;
}

