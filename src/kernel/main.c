/*=============================================================================
 * HeatOS Kernel Main
 * 32-bit Protected Mode entry point, called from entry.asm
 *===========================================================================*/
#include "vga.h"
#include "keyboard.h"

void kernel_main(void) {
    vga_init();
    keyboard_init();

    /* Desktop environment is being rewritten in standalone assembly */
    vga_write_at(10, 25, "HeatOS - Kernel Ready", VGA_ATTR(VGA_WHITE, VGA_BLUE));
    vga_write_at(12, 18, "Desktop environment is being rewritten in assembly.", VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLUE));

    for (;;)
        __asm__ volatile("hlt");
}
