/*=============================================================================
 * HeatOS Kernel Main
 * 32-bit Protected Mode entry point, called from entry.asm
 *===========================================================================*/
#include "vga.h"
#include "keyboard.h"
#include "terminal.h"
#include "popeye_plasma.h"

void kernel_main(void) {
    vga_init();
    keyboard_init();
    popeye_plasma_init();

    /* Always boot to terminal. Desktop is launched via 'popeye boot plasma'. */
    terminal_run();

    for (;;)
        __asm__ volatile("hlt");
}
