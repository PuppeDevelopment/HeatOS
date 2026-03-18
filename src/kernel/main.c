#include "fat32.h"
#include "gdt.h"
#include "graphics.h"
#include "idt.h"
#include "keyboard.h"
#include "klog.h"
#include "mm.h"
#include "panic.h"
#include "printf.h"
#include "scheduler.h"
#include "serial.h"
#include "tcp_stack.h"
#include "terminal.h"
#include "types.h"
#include "vga.h"
#include "web_stack.h"
#include "pit.h"

#define KERNEL_BOOT_MEM_BYTES 0x4000000u

void kernel_main(void) {
    serial_init();
    serial_write("HeatOS: kernel entry\n");
    kprintf("[boot] serial console online\n");

    klog_init();
    klog_info("boot", "kernel entry");

    gdt_init();
    klog_info("boot", "gdt initialized");

    idt_init();
    klog_info("boot", "idt initialized");

    pit_init();
    klog_info("boot", "pit timer initialized @100Hz");

    pmm_init(KERNEL_BOOT_MEM_BYTES);
    klog_log_u32(KLOG_INFO, "mm", "pmm free pages", pmm_free_blocks_count(), 10);
    kprintf("[mm] free pages=%u\n", pmm_free_blocks_count());
    BUG_ON(pmm_free_blocks_count() == 0, "no free pages after pmm_init");

    scheduler_init();
    klog_info("boot", "scheduler initialized");

    fat32_init();
    klog_info("boot", "fat32 initialized");

    net_stack_init();
    klog_info("boot", "network stack initialized");

    web_stack_init();
    klog_info("boot", "web stack initialized");

    vga_init();
    gfx_init();
    if (gfx_virtio_present()) {
        klog_info("gfx", "virtio display detected (graphics API active)");
    } else {
        klog_warn("gfx", "virtio display not detected, using VGA backend");
    }

    keyboard_init();
    klog_info("boot", "terminal start");

    terminal_run();
    for (;;)
        __asm__ volatile("hlt");
}
