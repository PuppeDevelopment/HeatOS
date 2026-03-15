/*=============================================================================
 * RushOS Kernel Main
 * 32-bit Protected Mode entry point, called from entry.asm
 *===========================================================================*/
#include "vga.h"
#include "keyboard.h"
#include "desktop.h"

void kernel_main(void) {
    vga_init();
    keyboard_init();

    /* Go straight to the Popeye Plasma desktop */
    desktop_run();

    /* Should never reach here */
    vga_clear(VGA_ATTR(VGA_WHITE, VGA_RED));
    vga_write_at(12, 30, "KERNEL PANIC", VGA_ATTR(VGA_WHITE, VGA_RED));
    for (;;)
        __asm__ volatile("hlt");
}
