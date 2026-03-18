#ifndef IDT_H
#define IDT_H
#include "types.h"
void idt_init(void);
void idt_register_handler(uint8_t num, void *handler_fn);
#endif
