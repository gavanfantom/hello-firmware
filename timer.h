/* timer.h */

#ifndef TIMER_H
#define TIMER_H

#define TIMER_PCLK 48000000

#define FRAME_TIMER   16,0
#define SPEAKER_TIMER 32,0
#define TIMEOUT_TIMER 16,1
#define SYSTIME_TIMER 32,1

#define FRAME_BASE       _TIMER_BASE(FRAME_TIMER)
#define FRAME_RESET     _TIMER_RESET(FRAME_TIMER)
#define FRAME_CLOCK     _TIMER_CLOCK(FRAME_TIMER)
#define FRAME_HANDLER _TIMER_HANDLER(FRAME_TIMER)
#define FRAME_IRQN       _TIMER_IRQN(FRAME_TIMER)
#define FRAME_MAX         _TIMER_MAX(FRAME_TIMER)
#define FRAME_PRESCALE  128

#define TIMEOUT_BASE       _TIMER_BASE(TIMEOUT_TIMER)
#define TIMEOUT_RESET     _TIMER_RESET(TIMEOUT_TIMER)
#define TIMEOUT_CLOCK     _TIMER_CLOCK(TIMEOUT_TIMER)
#define TIMEOUT_HANDLER _TIMER_HANDLER(TIMEOUT_TIMER)
#define TIMEOUT_IRQN       _TIMER_IRQN(TIMEOUT_TIMER)
#define TIMEOUT_MAX         _TIMER_MAX(TIMEOUT_TIMER)
#define TIMEOUT_PRESCALE  128

#define SYSTIME_BASE       _TIMER_BASE(SYSTIME_TIMER)
#define SYSTIME_RESET     _TIMER_RESET(SYSTIME_TIMER)
#define SYSTIME_CLOCK     _TIMER_CLOCK(SYSTIME_TIMER)
#define SYSTIME_HANDLER _TIMER_HANDLER(SYSTIME_TIMER)
#define SYSTIME_IRQN       _TIMER_IRQN(SYSTIME_TIMER)
#define SYSTIME_MAX         _TIMER_MAX(SYSTIME_TIMER)
#define SYSTIME_PRESCALE  1

#define SPEAKER_BASE       _TIMER_BASE(SPEAKER_TIMER)
#define SPEAKER_RESET     _TIMER_RESET(SPEAKER_TIMER)
#define SPEAKER_CLOCK     _TIMER_CLOCK(SPEAKER_TIMER)
#define SPEAKER_HANDLER _TIMER_HANDLER(SPEAKER_TIMER)
#define SPEAKER_PRESCALE  1

#define _TIMER_BASE(x)    __TIMER_BASE(x)
#define _TIMER_RESET(x)   __TIMER_RESET(x)
#define _TIMER_CLOCK(x)   __TIMER_CLOCK(x)
#define _TIMER_HANDLER(x) __TIMER_HANDLER(x)
#define _TIMER_IRQN(x)    __TIMER_IRQN(x)
#define _TIMER_MAX(x)     __TIMER_MAX(x)

#define __TIMER_BASE(x, y)    LPC_TIMER##x##_##y
#define __TIMER_RESET(x, y)   RESET_TIMER##y##_##x
#define __TIMER_CLOCK(x, y)   SYSCTL_CLOCK_CT##x##B##y
#define __TIMER_HANDLER(x, y) TIMER_##x##_##y##_IRQHandler
#define __TIMER_IRQN(x, y)    TIMER_##x##_##y##_IRQn
#define __TIMER_MAX(x, y)     ((1ULL<<(x)) - 1)

void frame_timer_init(void);
void frame_timer_on(int frequency);
void frame_timer_off(void);

void timeout_timer_init(void);
void timeout_timer_on(int period);
void timeout_timer_reset(void);
void timeout_timer_off(void);

void systime_timer_init(void);
void systime_timer_on(int period);
void systime_timer_off(void);
uint32_t systime_timer_get(void);

void delay(int ms);

#endif /* TIMER_H */
