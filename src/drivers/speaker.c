#include "speaker.h"
#include "io.h"
#include "pit.h"

/* PIT channel 2 drives the PC speaker. */
#define PIT_CMD        0x43u
#define PIT_CHANNEL2   0x42u
#define PC_SPEAKER_CTL 0x61u   /* Port B: bits 0=gate, 1=speaker enable */
#define PIT_BASE_FREQ  1193180u

void speaker_off(void) {
    uint8_t v = inb(PC_SPEAKER_CTL);
    outb(PC_SPEAKER_CTL, (uint8_t)(v & ~0x03u)); /* clear gate + speaker bits */
}

void speaker_beep(uint32_t freq_hz, uint32_t duration_ms) {
    if (freq_hz == 0 || freq_hz > 20000u) {
        speaker_off();
        return;
    }

    uint32_t divisor = PIT_BASE_FREQ / freq_hz;

    /* Program PIT channel 2: lo/hi byte, mode 3 (square wave). */
    outb(PIT_CMD, 0xB6u);
    outb(PIT_CHANNEL2, (uint8_t)(divisor & 0xFFu));
    outb(PIT_CHANNEL2, (uint8_t)(divisor >> 8));

    /* Connect channel 2 output to speaker (bits 0 and 1 of port 0x61). */
    outb(PC_SPEAKER_CTL, (uint8_t)(inb(PC_SPEAKER_CTL) | 0x03u));

    sleep_ms(duration_ms);

    speaker_off();
}
