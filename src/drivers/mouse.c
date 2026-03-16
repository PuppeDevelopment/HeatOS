#include "mouse.h"
#include "io.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT    0x64

#define PS2_STATUS_OBF  0x01
#define PS2_STATUS_IBF  0x02
#define PS2_STATUS_AUX  0x20

#define MOUSE_ACK       0xFA

static bool g_mouse_ready = false;
static bool g_reporting_enabled = false;
static uint8_t g_pkt[3];
static uint8_t g_pkt_idx = 0;

static bool ps2_wait_input_clear(void) {
    uint32_t guard = 100000u;
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_IBF) && guard--) {}
    return guard != 0u;
}

static bool ps2_wait_output_full(void) {
    uint32_t guard = 100000u;
    while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OBF) && guard--) {}
    return guard != 0u;
}

static void ps2_flush_output(void) {
    uint32_t guard = 64u;
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_OBF) && guard--) {
        (void)inb(PS2_DATA_PORT);
    }
}

static bool ps2_write_cmd(uint8_t cmd) {
    if (!ps2_wait_input_clear()) return false;
    outb(PS2_CMD_PORT, cmd);
    return true;
}

static bool mouse_send_cmd(uint8_t cmd) {
    if (!ps2_write_cmd(0xD4)) return false;
    if (!ps2_wait_input_clear()) return false;
    outb(PS2_DATA_PORT, cmd);

    if (!ps2_wait_output_full()) return false;
    uint8_t ack = inb(PS2_DATA_PORT);
    return ack == MOUSE_ACK;
}

bool mouse_init(void) {
    g_mouse_ready = false;
    g_reporting_enabled = false;
    g_pkt_idx = 0;

    ps2_flush_output();

    if (!ps2_write_cmd(0xA8)) return false; /* Enable auxiliary device */

    if (!ps2_write_cmd(0x20)) return false; /* Read controller command byte */
    if (!ps2_wait_output_full()) return false;
    uint8_t cmd_byte = inb(PS2_DATA_PORT);

    cmd_byte |= 0x02;               /* Enable IRQ12 routing */
    cmd_byte &= (uint8_t)~0x20u;    /* Enable mouse clock */

    if (!ps2_write_cmd(0x60)) return false; /* Write controller command byte */
    if (!ps2_wait_input_clear()) return false;
    outb(PS2_DATA_PORT, cmd_byte);

    if (!mouse_send_cmd(0xF6)) return false; /* Set defaults */
    if (!mouse_send_cmd(0xF4)) return false; /* Enable data reporting */

    g_mouse_ready = true;
    g_reporting_enabled = true;
    return true;
}

bool mouse_is_ready(void) {
    return g_mouse_ready;
}

void mouse_set_reporting(bool enabled) {
    if (!g_mouse_ready || enabled == g_reporting_enabled) return;

    if (enabled) {
        if (mouse_send_cmd(0xF4)) g_reporting_enabled = true;
    } else {
        if (mouse_send_cmd(0xF5)) {
            g_reporting_enabled = false;
            ps2_flush_output();
            g_pkt_idx = 0;
        }
    }
}

bool mouse_poll(mouse_packet_t *out) {
    if (!g_mouse_ready || !g_reporting_enabled) return false;

    uint8_t status = inb(PS2_STATUS_PORT);
    if (!(status & PS2_STATUS_OBF) || !(status & PS2_STATUS_AUX)) {
        return false;
    }

    uint8_t b = inb(PS2_DATA_PORT);

    if (g_pkt_idx == 0 && (b & 0x08u) == 0u) {
        return false;
    }

    g_pkt[g_pkt_idx++] = b;
    if (g_pkt_idx < 3) {
        return false;
    }

    g_pkt_idx = 0;

    int16_t dx = (g_pkt[0] & 0x10u) ? (int16_t)((int)g_pkt[1] - 256) : (int16_t)g_pkt[1];
    int16_t dy = (g_pkt[0] & 0x20u) ? (int16_t)((int)g_pkt[2] - 256) : (int16_t)g_pkt[2];

    if (g_pkt[0] & 0x40u) dx = 0; /* X overflow */
    if (g_pkt[0] & 0x80u) dy = 0; /* Y overflow */

    if (out) {
        out->dx = dx;
        out->dy = dy;
        out->left = (g_pkt[0] & 0x01u) != 0u;
        out->right = (g_pkt[0] & 0x02u) != 0u;
        out->middle = (g_pkt[0] & 0x04u) != 0u;
    }

    return true;
}
