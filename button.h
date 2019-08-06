/* button.h */

#ifndef BUTTON_H
#define BUTTON_H

#define BUTTON_UP     0x01
#define BUTTON_DOWN   0x02
#define BUTTON_LEFT   0x04
#define BUTTON_RIGHT  0x08
#define BUTTON_CENTRE 0x10

#define BUTTONS 5

void button_init(void);
int button_state(void);
int button_event(void);

#endif /* BUTTON_H */
