#include "pit.h"
#include "idt.h"
#include "io.h"

/* ---- Hardware constants ---- */
#define PIT_CHANNEL0   0x40u   /* PIT channel 0 data port          */
#define PIT_CMD        0x43u   /* PIT mode/command register        */
#define PIC_MASTER_CMD 0x20u   /* 8259A master PIC command port    */
#define PIC_MASTER_IMR 0x21u   /* 8259A master PIC mask register   */

#define PIT_HZ         100u
#define PIT_BASE_FREQ  1193180u
#define PIT_DIVISOR    (PIT_BASE_FREQ / PIT_HZ)   /* 11931 */
#define MS_PER_TICK    (1000u / PIT_HZ)            /* 10 ms */

/* ---- IRQ0 handler ---- */
static volatile uint32_t g_ticks = 0;

/* Minimal interrupt frame pushed by the CPU at ring 0->ring 0 transition. */
typedef struct {
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} pit_iframe_t;

static void __attribute__((interrupt)) pit_irq0(pit_iframe_t *frame) {
    (void)frame;
    g_ticks++;
    /* EOI to master PIC; keep ISR call-free to satisfy interrupt ABI checks. */
    __asm__ volatile("outb %0, $0x20" : : "a"((uint8_t)0x20u));
}

/* ---- Public API ---- */

void pit_init(void) {
    /* Program PIT channel 0: lo/hi byte access, mode 3 (square wave). */
    outb(PIT_CMD, 0x36u);
    outb(PIT_CHANNEL0, (uint8_t)(PIT_DIVISOR & 0xFFu));
    outb(PIT_CHANNEL0, (uint8_t)(PIT_DIVISOR >> 8));

    /* Register IRQ0 handler (INT 0x20) in IDT before unmasking. */
    idt_register_handler(0x20u, (void *)pit_irq0);

    /* Unmask IRQ0 in master PIC (clear bit 0 of IMR). */
    uint8_t mask = inb(PIC_MASTER_IMR);
    outb(PIC_MASTER_IMR, (uint8_t)(mask & ~0x01u));
}

uint32_t pit_ticks(void) {
    return g_ticks;
}

uint32_t pit_ms_since_boot(void) {
    return g_ticks * MS_PER_TICK;
}

uint32_t uptime_seconds(void) {
    return g_ticks / PIT_HZ;
}

/*
 * Spin (using HLT to be CPU-friendly) until at least `ms` milliseconds pass.
 * Requires interrupts to be enabled (they are after entry.asm's STI).
 */
void sleep_ms(uint32_t ms) {
    uint32_t ticks_needed = ms / MS_PER_TICK;
    if (ticks_needed == 0) ticks_needed = 1;
    uint32_t end = g_ticks + ticks_needed;
    while (g_ticks < end) {
        __asm__ volatile("hlt"); /* wake on next IRQ0 tick */
    }
}
