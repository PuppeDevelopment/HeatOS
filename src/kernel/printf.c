#include "printf.h"

#include "serial.h"

static int emit_char(printf_putc_fn out, void *ctx, char c) {
    if (!out) return 0;
    out(c, ctx);
    return 1;
}

static int emit_str(printf_putc_fn out, void *ctx, const char *s) {
    int written = 0;

    if (!s) s = "(null)";

    while (*s) {
        written += emit_char(out, ctx, *s++);
    }

    return written;
}

static int emit_u64_base(printf_putc_fn out, void *ctx,
                         uint64_t value, uint32_t base, bool upper) {
    char tmp[32];
    int pos = 0;
    int written = 0;

    if (base < 2 || base > 16) return 0;

    do {
        uint32_t digit = (uint32_t)(value % base);
        if (digit < 10) {
            tmp[pos++] = (char)('0' + digit);
        } else {
            tmp[pos++] = (char)((upper ? 'A' : 'a') + (digit - 10));
        }
        value /= base;
    } while (value && pos < (int)sizeof(tmp));

    while (pos > 0) {
        written += emit_char(out, ctx, tmp[--pos]);
    }

    return written;
}

int kvprintf(printf_putc_fn out, void *ctx, const char *fmt, va_list ap) {
    int written = 0;

    if (!fmt) return 0;

    while (*fmt) {
        if (*fmt != '%') {
            written += emit_char(out, ctx, *fmt++);
            continue;
        }

        fmt++;
        if (*fmt == '\0') break;

        switch (*fmt) {
            case '%':
                written += emit_char(out, ctx, '%');
                break;

            case 'c': {
                int c = va_arg(ap, int);
                written += emit_char(out, ctx, (char)c);
                break;
            }

            case 's': {
                const char *s = va_arg(ap, const char *);
                written += emit_str(out, ctx, s);
                break;
            }

            case 'd':
            case 'i': {
                int v = va_arg(ap, int);
                uint64_t uv;
                if (v < 0) {
                    written += emit_char(out, ctx, '-');
                    uv = (uint64_t)(-(int64_t)v);
                } else {
                    uv = (uint64_t)v;
                }
                written += emit_u64_base(out, ctx, uv, 10, false);
                break;
            }

            case 'u': {
                unsigned int v = va_arg(ap, unsigned int);
                written += emit_u64_base(out, ctx, (uint64_t)v, 10, false);
                break;
            }

            case 'x': {
                unsigned int v = va_arg(ap, unsigned int);
                written += emit_u64_base(out, ctx, (uint64_t)v, 16, false);
                break;
            }

            case 'X': {
                unsigned int v = va_arg(ap, unsigned int);
                written += emit_u64_base(out, ctx, (uint64_t)v, 16, true);
                break;
            }

            case 'p': {
                uintptr_t v = va_arg(ap, uintptr_t);
                written += emit_str(out, ctx, "0x");
                written += emit_u64_base(out, ctx, (uint64_t)v, 16, false);
                break;
            }

            default:
                written += emit_char(out, ctx, '%');
                written += emit_char(out, ctx, *fmt);
                break;
        }

        fmt++;
    }

    return written;
}

typedef struct {
    char *buf;
    size_t cap;
    size_t pos;
} snprintf_ctx_t;

static void snprintf_putc(char c, void *ctx_ptr) {
    snprintf_ctx_t *ctx = (snprintf_ctx_t *)ctx_ptr;

    if (!ctx) return;

    if (ctx->cap > 0 && ctx->pos + 1 < ctx->cap) {
        ctx->buf[ctx->pos] = c;
    }

    ctx->pos++;
}

int kvsnprintf(char *buf, size_t cap, const char *fmt, va_list ap) {
    snprintf_ctx_t ctx;
    int total;

    if (!buf && cap > 0) return 0;

    ctx.buf = buf;
    ctx.cap = cap;
    ctx.pos = 0;

    total = kvprintf(snprintf_putc, &ctx, fmt, ap);

    if (cap > 0) {
        size_t term = (ctx.pos < cap) ? ctx.pos : (cap - 1);
        ctx.buf[term] = '\0';
    }

    return total;
}

int ksnprintf(char *buf, size_t cap, const char *fmt, ...) {
    va_list ap;
    int total;

    va_start(ap, fmt);
    total = kvsnprintf(buf, cap, fmt, ap);
    va_end(ap);

    return total;
}

static void serial_printf_putc(char c, void *ctx) {
    (void)ctx;
    serial_putc(c);
}

int kprintf(const char *fmt, ...) {
    va_list ap;
    int total;

    va_start(ap, fmt);
    total = kvprintf(serial_printf_putc, 0, fmt, ap);
    va_end(ap);

    return total;
}
