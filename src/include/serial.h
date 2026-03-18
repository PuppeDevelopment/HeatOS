#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

void serial_init(void);
bool serial_is_ready(void);
void serial_putc(char c);
void serial_write(const char *s);

#endif
