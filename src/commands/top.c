#include "terminal.h"
#include "scheduler.h"
#include "string.h"
#include "pit.h"
#include "mm.h"

static const char *state_name(uint32_t state) {
    switch (state) {
        case 1: return "running";
        case 0: return "stopped";
        default: return "unknown";
    }
}

void cmd_top(const char *args) {
    (void)args;

    scheduler_proc_info_t procs[32];
    int count = scheduler_snapshot(procs, 32);

    term_puts("HeatOS top\n");
    /* ---- Uptime ---- */
    uint32_t secs = uptime_seconds();
    uint32_t h = secs / 3600u, m = (secs % 3600u) / 60u, s = secs % 60u;
    char nbuf[16];
    term_puts("Uptime:  ");
    itoa((int)h, nbuf, 10); term_puts(nbuf); term_puts("h ");
    itoa((int)m, nbuf, 10); term_puts(nbuf); term_puts("m ");
    itoa((int)s, nbuf, 10); term_puts(nbuf); term_puts("s");
    term_puts("  (ticks: ");
    itoa((int)pit_ticks(), nbuf, 10); term_puts(nbuf);
    term_puts(")\n");

    /* ---- Memory ---- */
    uint32_t used_kb  = kheap_used_bytes() / 1024u;
    uint32_t free_kb  = kheap_free_bytes() / 1024u;
    uint32_t pmm_free = pmm_free_blocks_count();
    term_puts("Heap:    ");
    itoa((int)used_kb, nbuf, 10); term_puts(nbuf); term_puts(" KB used  /  ");
    itoa((int)free_kb, nbuf, 10); term_puts(nbuf); term_puts(" KB free\n");
    term_puts("Pages:   ");
    itoa((int)pmm_free, nbuf, 10); term_puts(nbuf); term_puts(" free (4 KB each)\n");

    /* ---- Processes ---- */
    term_puts("Procs:   ");
    itoa((int)scheduler_process_count(), nbuf, 10); term_puts(nbuf);
    term_puts("  current PID: ");
    itoa((int)scheduler_current_pid(), nbuf, 10); term_puts(nbuf);
    term_puts("\n\n");

    term_puts("PID  STATE\n");
    for (int i = 0; i < count; i++) {
        itoa((int)procs[i].id, nbuf, 10);
        term_puts(nbuf);
        if (procs[i].id < 10) term_puts("    ");
        else if (procs[i].id < 100) term_puts("   ");
        else term_puts("  ");
        term_puts(state_name(procs[i].state));
        if (procs[i].current) term_puts(" *");
        term_puts("\n");
    }
}
