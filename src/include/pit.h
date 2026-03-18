#ifndef PIT_H
#define PIT_H

#include "types.h"

/* Initialize PIT channel 0 at 100 Hz and install IRQ0 handler. */
void     pit_init(void);

/* Raw 100 Hz tick counter (incremented by IRQ0 handler). */
uint32_t pit_ticks(void);

/* Milliseconds elapsed since pit_init() was called. */
uint32_t pit_ms_since_boot(void);

/* Whole seconds since pit_init() was called. */
uint32_t uptime_seconds(void);

/* Busy-wait (HLT-idle) for at least `ms` milliseconds. */
void     sleep_ms(uint32_t ms);

#endif
