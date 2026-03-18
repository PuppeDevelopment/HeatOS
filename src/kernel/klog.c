#include "klog.h"
#include "serial.h"
#include "string.h"

#define KLOG_RING_CAPACITY 8192

static char g_klog_ring[KLOG_RING_CAPACITY];
static uint32_t g_klog_head = 0;
static uint32_t g_klog_size = 0;
static uint32_t g_klog_lines = 0;

static char klog_level_char(klog_level_t level) {
    switch (level) {
        case KLOG_EMERG: return '0';
        case KLOG_ALERT: return '1';
        case KLOG_CRIT: return '2';
        case KLOG_ERR: return '3';
        case KLOG_WARN: return '4';
        case KLOG_NOTICE: return '5';
        case KLOG_INFO: return '6';
        case KLOG_DEBUG: return '7';
        default: return '?';
    }
}

static void klog_ring_putc(char c) {
    g_klog_ring[g_klog_head] = c;
    g_klog_head = (g_klog_head + 1u) % KLOG_RING_CAPACITY;

    if (g_klog_size < KLOG_RING_CAPACITY) {
        g_klog_size++;
    }
}

static void klog_ring_puts(const char *s) {
    if (!s) return;
    while (*s) {
        klog_ring_putc(*s++);
    }
}

static void klog_append(char *dst, size_t cap, const char *src) {
    size_t len;

    if (!dst || cap == 0 || !src) return;

    len = strlen(dst);
    while (len + 1 < cap && *src) {
        dst[len++] = *src++;
    }
    dst[len] = '\0';
}

void klog_init(void) {
    g_klog_head = 0;
    g_klog_size = 0;
    g_klog_lines = 0;
    memset(g_klog_ring, 0, sizeof(g_klog_ring));
}

void klog_clear(void) {
    g_klog_head = 0;
    g_klog_size = 0;
    memset(g_klog_ring, 0, sizeof(g_klog_ring));
}

void klog_log(klog_level_t level, const char *subsys, const char *msg) {
    char seq_buf[16];
    char level_c;

    if (!subsys || !*subsys) subsys = "core";
    if (!msg) msg = "";

    g_klog_lines++;
    utoa(g_klog_lines, seq_buf, 10);
    level_c = klog_level_char(level);

    klog_ring_putc('<');
    klog_ring_putc(level_c);
    klog_ring_putc('>');
    klog_ring_putc(' ');
    klog_ring_putc('[');
    klog_ring_puts(seq_buf);
    klog_ring_putc(']');
    klog_ring_putc(' ');
    klog_ring_putc('[');
    klog_ring_puts(subsys);
    klog_ring_putc(']');
    klog_ring_putc(' ');
    klog_ring_puts(msg);
    klog_ring_putc('\n');

    if (serial_is_ready()) {
        serial_putc('<');
        serial_putc(level_c);
        serial_putc('>');
        serial_putc(' ');
        serial_putc('[');
        serial_write(seq_buf);
        serial_putc(']');
        serial_putc(' ');
        serial_putc('[');
        serial_write(subsys);
        serial_putc(']');
        serial_putc(' ');
        serial_write(msg);
        serial_putc('\n');
    }
}

void klog_log_u32(klog_level_t level, const char *subsys, const char *prefix, uint32_t value, int base) {
    char value_buf[16];
    char line[96];

    if (base != 16) base = 10;

    line[0] = '\0';
    if (prefix && *prefix) {
        klog_append(line, sizeof(line), prefix);
    }

    if (base == 16) {
        if (line[0]) klog_append(line, sizeof(line), " ");
        klog_append(line, sizeof(line), "0x");
        utoa(value, value_buf, 16);
    } else {
        if (line[0]) klog_append(line, sizeof(line), " ");
        utoa(value, value_buf, 10);
    }

    klog_append(line, sizeof(line), value_buf);
    klog_log(level, subsys, line);
}

int klog_read(char *out, int max_len) {
    int copy_len;
    uint32_t start;

    if (!out || max_len <= 0) return 0;

    if (g_klog_size == 0) {
        out[0] = '\0';
        return 0;
    }

    copy_len = (int)g_klog_size;
    if (copy_len > (max_len - 1)) {
        copy_len = max_len - 1;
    }

    start = (g_klog_head + KLOG_RING_CAPACITY - g_klog_size) % KLOG_RING_CAPACITY;
    for (int i = 0; i < copy_len; i++) {
        out[i] = g_klog_ring[(start + (uint32_t)i) % KLOG_RING_CAPACITY];
    }

    out[copy_len] = '\0';
    return copy_len;
}

uint32_t klog_bytes_used(void) {
    return g_klog_size;
}

uint32_t klog_line_count(void) {
    return g_klog_lines;
}