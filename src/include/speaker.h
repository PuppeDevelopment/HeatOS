#ifndef SPEAKER_H
#define SPEAKER_H

#include "types.h"

/* Play a tone at `freq_hz` for `duration_ms` milliseconds via PC speaker.
 * Requires pit_init() to have been called (uses sleep_ms internally). */
void speaker_beep(uint32_t freq_hz, uint32_t duration_ms);

/* Silence the PC speaker immediately. */
void speaker_off(void);

#endif
