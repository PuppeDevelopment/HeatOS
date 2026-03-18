#ifndef PRINTF_H
#define PRINTF_H

#include "types.h"
#include <stdarg.h>

typedef void (*printf_putc_fn)(char c, void *ctx);

int kvprintf(printf_putc_fn out, void *ctx, const char *fmt, va_list ap);
int kprintf(const char *fmt, ...);

int kvsnprintf(char *buf, size_t cap, const char *fmt, va_list ap);
int ksnprintf(char *buf, size_t cap, const char *fmt, ...);

#endif
