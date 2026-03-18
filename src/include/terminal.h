#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"

typedef struct {
    void (*putc)(char c, uint8_t attr);
    void (*puts)(const char *s);
    void (*putln)(const char *s);
} term_hooks_t;

void term_putc(char c, uint8_t attr);
void term_puts(const char *s);
void term_reset_screen(void);
void terminal_run(void);
void term_print_history(void);

#endif
