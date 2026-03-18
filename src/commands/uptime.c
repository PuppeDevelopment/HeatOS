#include "terminal.h"
#include "pit.h"
#include "string.h"

void cmd_uptime(const char *args) {
    (void)args;
    uint32_t secs = uptime_seconds();
    uint32_t h   = secs / 3600u;
    uint32_t m   = (secs % 3600u) / 60u;
    uint32_t s   = secs % 60u;

    char buf[8];
    term_puts("Uptime: ");

    itoa((int)h, buf, 10); term_puts(buf); term_puts("h ");
    itoa((int)m, buf, 10); term_puts(buf); term_puts("m ");
    itoa((int)s, buf, 10); term_puts(buf); term_puts("s");

    term_puts("  (");
    itoa((int)pit_ms_since_boot(), buf, 10);
    term_puts(buf);
    term_puts(" ms since boot)\n");
}
