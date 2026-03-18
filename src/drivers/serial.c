#include "serial.h"

#include "io.h"

#define SERIAL_COM1 0x3F8

static bool g_serial_ready = false;

static bool serial_tx_ready(void) {
    return (inb(SERIAL_COM1 + 5) & 0x20u) != 0u;
}

void serial_init(void) {
    outb(SERIAL_COM1 + 1, 0x00); /* Disable all UART interrupts. */
    outb(SERIAL_COM1 + 3, 0x80); /* Enable divisor latch. */
    outb(SERIAL_COM1 + 0, 0x03); /* 38400 baud (divisor 3). */
    outb(SERIAL_COM1 + 1, 0x00);
    outb(SERIAL_COM1 + 3, 0x03); /* 8 data bits, no parity, 1 stop bit. */
    outb(SERIAL_COM1 + 2, 0xC7); /* Enable FIFO, clear, 14-byte threshold. */
    outb(SERIAL_COM1 + 4, 0x0B); /* IRQs enabled, RTS/DSR set. */

    g_serial_ready = true;
}

bool serial_is_ready(void) {
    return g_serial_ready;
}

void serial_putc(char c) {
    uint32_t guard = 0;

    if (!g_serial_ready) return;

    if (c == '\n') {
        serial_putc('\r');
    }

    while (!serial_tx_ready()) {
        guard++;
        if (guard > 1000000u) {
            return;
        }
    }

    outb(SERIAL_COM1, (uint8_t)c);
}

void serial_write(const char *s) {
    if (!s) return;

    while (*s) {
        serial_putc(*s++);
    }
}
