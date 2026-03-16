#ifndef TERMINAL_H
#define TERMINAL_H

void terminal_run(void);

void term_putc(char c, uint8_t attr);
void term_puts(const char *s);
void term_putln(const char *s);

#endif
