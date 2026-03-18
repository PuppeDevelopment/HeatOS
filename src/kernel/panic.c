#include "panic.h"
#include "klog.h"
#include "string.h"
#include "vga.h"

void kernel_panic(const char *msg, const char *file, int line) {
    char line_buf[16];

    if (!msg) msg = "fatal kernel error";
    if (!file) file = "unknown";

    __asm__ volatile("cli");

    klog_log(KLOG_CRIT, "panic", msg);

    vga_clear(VGA_ATTR(VGA_WHITE, VGA_RED));
    vga_write_at(1, 2, "KERNEL PANIC", VGA_ATTR(VGA_YELLOW, VGA_RED));
    vga_write_at(3, 2, "Reason:", VGA_ATTR(VGA_WHITE, VGA_RED));
    vga_write_at(3, 10, msg, VGA_ATTR(VGA_WHITE, VGA_RED));

    vga_write_at(5, 2, "Location:", VGA_ATTR(VGA_WHITE, VGA_RED));
    vga_write_at(5, 12, file, VGA_ATTR(VGA_WHITE, VGA_RED));
    vga_write_at(5, 12 + (int)strlen(file), ":", VGA_ATTR(VGA_WHITE, VGA_RED));
    itoa(line, line_buf, 10);
    vga_write_at(5, 13 + (int)strlen(file), line_buf, VGA_ATTR(VGA_WHITE, VGA_RED));

    vga_write_at(7, 2, "System halted. Restart required.", VGA_ATTR(VGA_WHITE, VGA_RED));

    for (;;) {
        __asm__ volatile("hlt");
    }
}