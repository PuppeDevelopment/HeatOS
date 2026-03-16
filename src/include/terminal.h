#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"

typedef struct {
    void (*putc)(char c, uint8_t attr);
    void (*puts)(const char *s);
    void (*putln)(const char *s);
} term_hooks_t;

void terminal_run(void);

#endif
