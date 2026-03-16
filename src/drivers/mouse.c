#include "mouse.h"
#include "io.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT    0x64

#define PS2_STATUS_OBF  0x01
#define PS2_STATUS_IBF  0x02
#define PS2_STATUS_AUX  0x20

#define MOUSE_ACK            0xFA
#define MOUSE_RESEND         0xFE
#define MOUSE_BAT_OK         0xAA
#define MOUSE_DEVICE_STD     0x00
#define MOUSE_DEVICE_WHEEL   0x03
#define MOUSE_DEVICE_FIVEBTN 0x04

#define MOUSE_QUEUE_SIZE 32

static bool g_mouse_ready = false;
static bool g_reporting_enabled = false;
static uint8_t g_pkt[3];
static uint8_t g_pkt_idx = 0;
static mouse_packet_t g_queue[MOUSE_QUEUE_SIZE];
static uint8_t g_queue_head = 0;
static uint8_t g_queue_tail = 0;

static bool ps2_wait_input_clear(void) {
    uint32_t guard = 100000u;
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_IBF) && guard--) {}
    return guard != 0u;
}

static bool ps2_read_aux_byte(uint8_t *out) {
    uint32_t guard = 100000u;

    while (guard--) {
        uint8_t status = inb(PS2_STATUS_PORT);
        if ((status & PS2_STATUS_OBF) == 0u) {
            continue;
        }

        if ((status & PS2_STATUS_AUX) == 0u) {
            (void)inb(PS2_DATA_PORT);
            continue;
        }

        if (out) {
            *out = inb(PS2_DATA_PORT);
        } else {
            (void)inb(PS2_DATA_PORT);
        }
        return true;
    }

    return false;
}

static void ps2_flush_output(void) {
    uint32_t guard = 128u;
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_OBF) && guard--) {
        (void)inb(PS2_DATA_PORT);
    }
}

static bool ps2_write_cmd(uint8_t cmd) {
    if (!ps2_wait_input_clear()) return false;
    outb(PS2_CMD_PORT, cmd);
    return true;
}

static bool ps2_write_aux(uint8_t value) {
    if (!ps2_write_cmd(0xD4)) return false;
    if (!ps2_wait_input_clear()) return false;
    outb(PS2_DATA_PORT, value);
    return true;
}

static bool mouse_expect_ack(void) {
    uint8_t reply = 0;

    for (int attempt = 0; attempt < 4; attempt++) {
        if (!ps2_read_aux_byte(&reply)) {
            return false;
        }

        if (reply == MOUSE_ACK) {
            return true;
        }

        if (reply == MOUSE_RESEND) {
            return false;
        }
    }

    return false;
}

static bool mouse_send_cmd(uint8_t cmd) {
    for (int attempt = 0; attempt < 3; attempt++) {
        if (!ps2_write_aux(cmd)) {
            return false;
        }

        if (mouse_expect_ack()) {
            return true;
        }
    }

    return false;
}

static bool mouse_send_cmd_arg(uint8_t cmd, uint8_t arg) {
    for (int attempt = 0; attempt < 3; attempt++) {
        if (!ps2_write_aux(cmd)) {
            return false;
        }
        if (!mouse_expect_ack()) {
            continue;
        }

        if (!ps2_write_aux(arg)) {
            return false;
        }
        if (mouse_expect_ack()) {
            return true;
        }
    }

    return false;
}

static void mouse_queue_reset(void) {
    g_queue_head = 0;
    g_queue_tail = 0;
}

static bool mouse_queue_empty(void) {
    return g_queue_head == g_queue_tail;
}

static void mouse_queue_push(const mouse_packet_t *pkt) {
    uint8_t next = (uint8_t)((g_queue_head + 1u) % MOUSE_QUEUE_SIZE);
    if (next == g_queue_tail) {
        g_queue_tail = (uint8_t)((g_queue_tail + 1u) % MOUSE_QUEUE_SIZE);
    }

    g_queue[g_queue_head] = *pkt;
    g_queue_head = next;
}

static bool mouse_queue_pop(mouse_packet_t *pkt) {
    if (mouse_queue_empty()) {
        return false;
    }

    if (pkt) {
        *pkt = g_queue[g_queue_tail];
    }
    g_queue_tail = (uint8_t)((g_queue_tail + 1u) % MOUSE_QUEUE_SIZE);
    return true;
}

static bool mouse_reset_device(void) {
    uint8_t value = 0;
    bool saw_ack = false;

    if (!ps2_write_aux(0xFF)) return false;

    for (int attempt = 0; attempt < 4; attempt++) {
        if (!ps2_read_aux_byte(&value)) {
            break;
        }

        if (value == MOUSE_ACK) {
            saw_ack = true;
            continue;
        }

        if (value == MOUSE_RESEND) {
            return false;
        }

        if (value == MOUSE_BAT_OK ||
            value == MOUSE_DEVICE_STD ||
            value == MOUSE_DEVICE_WHEEL ||
            value == MOUSE_DEVICE_FIVEBTN) {
            continue;
        }
    }

    return saw_ack;
}

static void mouse_drain_device(void) {
    while (1) {
        uint8_t status = inb(PS2_STATUS_PORT);
        uint8_t b;

        if ((status & PS2_STATUS_OBF) == 0u || (status & PS2_STATUS_AUX) == 0u) {
            break;
        }

        b = inb(PS2_DATA_PORT);

        if (b == MOUSE_ACK || b == MOUSE_RESEND || b == MOUSE_BAT_OK ||
            b == MOUSE_DEVICE_STD || b == MOUSE_DEVICE_WHEEL || b == MOUSE_DEVICE_FIVEBTN) {
            continue;
        }

        if (!g_mouse_ready || !g_reporting_enabled) {
            continue;
        }

        if (g_pkt_idx == 0u && (b & 0x08u) == 0u) {
            continue;
        }

        g_pkt[g_pkt_idx++] = b;
        if (g_pkt_idx == 3u) {
            mouse_packet_t pkt;
            int16_t dx = (g_pkt[0] & 0x10u) ? (int16_t)((int)g_pkt[1] - 256) : (int16_t)g_pkt[1];
            int16_t dy = (g_pkt[0] & 0x20u) ? (int16_t)((int)g_pkt[2] - 256) : (int16_t)g_pkt[2];

            if (g_pkt[0] & 0x40u) dx = 0;
            if (g_pkt[0] & 0x80u) dy = 0;

            pkt.dx = dx;
            pkt.dy = dy;
            pkt.left = (g_pkt[0] & 0x01u) != 0u;
            pkt.right = (g_pkt[0] & 0x02u) != 0u;
            pkt.middle = (g_pkt[0] & 0x04u) != 0u;

            mouse_queue_push(&pkt);
            g_pkt_idx = 0u;
        }
    }
}

bool mouse_init(void) {
    uint8_t cmd_byte = 0;
    bool device_replied = false;

    g_mouse_ready = false;
    g_reporting_enabled = false;
    g_pkt_idx = 0u;
    mouse_queue_reset();
    ps2_flush_output();

    if (!ps2_write_cmd(0xA8)) return false;
    if (!ps2_write_cmd(0x20)) return false;
    if (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OBF)) {
        uint32_t guard = 100000u;
        while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OBF) && guard--) {}
        if (guard == 0u) return false;
    }
    cmd_byte = inb(PS2_DATA_PORT);

    cmd_byte |= 0x02u;
    cmd_byte &= (uint8_t)~0x20u;

    if (!ps2_write_cmd(0x60)) return false;
    if (!ps2_wait_input_clear()) return false;
    outb(PS2_DATA_PORT, cmd_byte);

    ps2_flush_output();

    device_replied = mouse_reset_device();

    /* PS/2 mice in QEMU/older VMs can miss one of these ACKs even though the
       device still works. Use best-effort configuration and rely on runtime
       packet sync instead of treating a missed reply as fatal. */
    if (mouse_send_cmd(0xF5)) device_replied = true;
    if (mouse_send_cmd(0xF6)) device_replied = true;
    (void)mouse_send_cmd_arg(0xE8, 2u);
    (void)mouse_send_cmd_arg(0xF3, 80u);

    g_mouse_ready = true;
    g_reporting_enabled = false;
    g_pkt_idx = 0u;
    mouse_queue_reset();
    ps2_flush_output();
    (void)device_replied;
    return true;
}

bool mouse_is_ready(void) {
    return g_mouse_ready;
}

void mouse_set_reporting(bool enabled) {
    if (!g_mouse_ready || enabled == g_reporting_enabled) {
        return;
    }

    if (enabled) {
        (void)mouse_send_cmd(0xF4);
        g_reporting_enabled = true;
        g_pkt_idx = 0u;
        mouse_queue_reset();
    } else {
        (void)mouse_send_cmd(0xF5);
        g_reporting_enabled = false;
        g_pkt_idx = 0u;
        mouse_queue_reset();
        ps2_flush_output();
    }
}

bool mouse_poll(mouse_packet_t *out) {
    mouse_drain_device();
    return mouse_queue_pop(out);
}
