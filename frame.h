/* frame.h */

#ifndef FRAME_H
#define FRAME_H

#define IDLE_FRAME_RATE 50

extern uint8_t *screen_buf;

void frame_init(void);

#define DISPLAY_NOTHING 0
#define DISPLAY_IMAGE 1
#define DISPLAY_VIDEO 2

extern int displaytype;

void set_frame_rate(int frame_rate);

void frame_update(void);

#endif /* FRAME_H */
