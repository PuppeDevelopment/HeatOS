#include "terminal.h"
#include "graphics.h"
#include "mm.h"
#include "scheduler.h"
#include "klog.h"
#include "string.h"

static void print_u32_line(const char *label, uint32_t value) {
    char nbuf[16];
    term_puts(label);
    utoa(value, nbuf, 10);
    term_puts(nbuf);
    term_puts("\n");
}

void cmd_kstats(const char *args) {
    (void)args;

    term_puts("Kernel Stats\n");
    term_puts("------------\n");

    print_u32_line("PMM pages total: ", pmm_total_blocks());
    print_u32_line("PMM pages used:  ", pmm_used_blocks_count());
    print_u32_line("PMM pages free:  ", pmm_free_blocks_count());

    print_u32_line("Heap bytes used: ", kheap_used_bytes());
    print_u32_line("Heap bytes free: ", kheap_free_bytes());

    print_u32_line("Scheduler procs: ", scheduler_process_count());
    print_u32_line("Scheduler ticks: ", scheduler_tick_count());

    term_puts("Graphics backend: ");
    term_puts(gfx_backend_name());
    term_puts("\n");
    term_puts("Virtio display:  ");
    term_puts(gfx_virtio_present() ? "detected" : "not detected");
    term_puts("\n");

    print_u32_line("klog lines:      ", klog_line_count());
    print_u32_line("klog bytes:      ", klog_bytes_used());
}
