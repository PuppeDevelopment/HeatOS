#include "terminal.h"
#include "speaker.h"
#include "string.h"

/* beep [freq_hz [duration_ms]]
 * Defaults: 440 Hz (concert A), 200 ms.
 * Requires QEMU to have audio output configured. */
void cmd_beep(const char *args) {
    uint32_t freq = 440u;
    uint32_t dur  = 200u;

    if (args && *args) {
        /* Parse first number (frequency) */
        uint32_t v = 0;
        const char *p = args;
        while (*p >= '0' && *p <= '9') { v = v * 10u + (uint32_t)(*p - '0'); p++; }
        if (v > 0) freq = v;

        /* Skip whitespace, parse second number (duration) */
        while (*p == ' ') p++;
        v = 0;
        while (*p >= '0' && *p <= '9') { v = v * 10u + (uint32_t)(*p - '0'); p++; }
        if (v > 0) dur = v;
    }

    speaker_beep(freq, dur);
}
