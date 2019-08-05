/* speaker.h */

#ifndef SPEAKER_H
#define SPEAKER_H

void speaker_init(void);
void speaker_on(int frequency);
void speaker_off(void);
void beep(int frequency, int duration);
void beep_up(void);
void beep_down(void);

#endif /* SPEAKER_H */
