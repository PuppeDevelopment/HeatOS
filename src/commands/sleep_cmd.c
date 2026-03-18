#include "terminal.h"
#include "pit.h"
#include "string.h"

void cmd_sleep(const char *args) {
    if (!args || !*args) {
        term_puts("Usage: sleep <milliseconds>\n");
        return;
    }
    uint32_t ms = 0;
    const char *p = args;
    while (*p >= '0' && *p <= '9') {
        ms = ms * 10u + (uint32_t)(*p - '0');
        p++;
    }
    if (ms == 0) {
        term_puts("sleep: invalid duration\n");
        return;
    }
    sleep_ms(ms);
}
