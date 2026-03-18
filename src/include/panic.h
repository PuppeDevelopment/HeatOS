#ifndef PANIC_H
#define PANIC_H

#include "types.h"

void kernel_panic(const char *msg, const char *file, int line);

#define PANIC(msg) kernel_panic((msg), __FILE__, __LINE__)

#define BUG_ON(cond, msg) \
    do { \
        if (cond) kernel_panic((msg), __FILE__, __LINE__); \
    } while (0)

#define KASSERT(cond, msg) BUG_ON(!(cond), (msg))

#endif