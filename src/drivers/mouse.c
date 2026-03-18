#include "mouse.h"

#include "io.h"
#include "vga.h"

#define KB_DATA_PORT   0x60
#define KB_STATUS_PORT 0x64
#define KB_STATUS_OBF  0x01
#define KB_STATUS_IBF  0x02
#define KB_STATUS_AUX  0x20

static int g_mouse_x = VGA_WIDTH / 2;
static int g_mouse_y = VGA_HEIGHT / 2;
static int g_dx_accum = 0;
static int g_dy_accum = 0;
static uint8_t g_buttons = 0;
static bool g_has_event = false;
static bool g_ready = false;

static uint8_t g_packet[3];
static int g_packet_idx = 0;

static int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static bool ps2_wait_input_clear(uint32_t guard) {
    while (guard--) {
        if ((inb(KB_STATUS_PORT) & KB_STATUS_IBF) == 0) return true;
    }
    return false;
}

static bool ps2_wait_output_full(uint32_t guard) {
    while (guard--) {
        if (inb(KB_STATUS_PORT) & KB_STATUS_OBF) return true;
    }
    return false;
}

static bool mouse_read_data(uint8_t *value) {
    if (!value) return false;
    if (!ps2_wait_output_full(800000u)) return false;
    *value = inb(KB_DATA_PORT);
    return true;
}

static bool mouse_write_device(uint8_t value) {
    uint8_t ack;

    if (!ps2_wait_input_clear(800000u)) return false;
    outb(KB_STATUS_PORT, 0xD4);
    if (!ps2_wait_input_clear(800000u)) return false;
    outb(KB_DATA_PORT, value);

    if (!mouse_read_data(&ack)) return false;
    return ack == 0xFAu;
}

static void mouse_process_packet(uint8_t b0, uint8_t b1, uint8_t b2) {
    if (b0 & 0xC0u) {
        return;
    }

    int dx = (int)(int8_t)b1;
    int dy = (int)(int8_t)b2;

    g_dx_accum += dx;
    g_dy_accum -= dy;

    g_mouse_x = clampi(g_mouse_x + dx, 0, VGA_WIDTH - 1);
    g_mouse_y = clampi(g_mouse_y - dy, 0, VGA_HEIGHT - 1);

    uint8_t new_buttons = (uint8_t)(b0 & 0x07u);
    if (dx || dy || new_buttons != g_buttons) {
        g_has_event = true;
    }
    g_buttons = new_buttons;
}

int mouse_init(void) {
    uint8_t cmd;

    g_mouse_x = VGA_WIDTH / 2;
    g_mouse_y = VGA_HEIGHT / 2;
    g_dx_accum = 0;
    g_dy_accum = 0;
    g_buttons = 0;
    g_has_event = false;
    g_packet_idx = 0;
    g_ready = false;

    if (!ps2_wait_input_clear(800000u)) return 0;
    outb(KB_STATUS_PORT, 0xA8); /* enable auxiliary device */

    if (!ps2_wait_input_clear(800000u)) return 0;
    outb(KB_STATUS_PORT, 0x20); /* read command byte */
    if (!mouse_read_data(&cmd)) return 0;

    cmd |= 0x02u;   /* enable IRQ12 */
    cmd &= (uint8_t)~0x20u; /* enable mouse clock */

    if (!ps2_wait_input_clear(800000u)) return 0;
    outb(KB_STATUS_PORT, 0x60); /* write command byte */
    if (!ps2_wait_input_clear(800000u)) return 0;
    outb(KB_DATA_PORT, cmd);

    if (!mouse_write_device(0xF6u)) return 0; /* defaults */
    if (!mouse_write_device(0xF4u)) return 0; /* enable streaming */

    g_ready = true;
    return 1;
}

void mouse_handle_aux_byte(uint8_t data) {
    if (!g_ready) return;

    if (g_packet_idx == 0 && (data & 0x08u) == 0) {
        return;
    }

    g_packet[g_packet_idx++] = data;
    if (g_packet_idx >= 3) {
        mouse_process_packet(g_packet[0], g_packet[1], g_packet[2]);
        g_packet_idx = 0;
    }
}

void mouse_poll(void) {
    while (inb(KB_STATUS_PORT) & KB_STATUS_OBF) {
        uint8_t status = inb(KB_STATUS_PORT);
        if ((status & KB_STATUS_AUX) == 0) break;
        mouse_handle_aux_byte(inb(KB_DATA_PORT));
    }
}

void mouse_get_screen_pos(int *x, int *y) {
    if (x) *x = g_mouse_x;
    if (y) *y = g_mouse_y;
}

uint8_t mouse_get_buttons(void) {
    return g_buttons;
}

bool mouse_has_event(void) {
    return g_has_event;
}

void mouse_clear_event(void) {
    g_has_event = false;
}

void mouse_get_delta(int *dx, int *dy) {
    if (dx) *dx = g_dx_accum;
    if (dy) *dy = g_dy_accum;
    g_dx_accum = 0;
    g_dy_accum = 0;
}

void mouse_set_screen_pos(int x, int y) {
    g_mouse_x = clampi(x, 0, VGA_WIDTH - 1);
    g_mouse_y = clampi(y, 0, VGA_HEIGHT - 1);
}
