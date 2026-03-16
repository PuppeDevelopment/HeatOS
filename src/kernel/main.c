/*=============================================================================
 * HeatOS Kernel Main
 * 32-bit Protected Mode entry point, called from entry.asm
 *===========================================================================*/
#include "vga.h"
#include "keyboard.h"
#include "mouse.h"
#include "ramdisk.h"
#include "terminal.h"
#include "popeye_plasma.h"

void kernel_main(void) {
    vga_init();
    keyboard_init();
    if (mouse_init()) {
        mouse_set_reporting(false);
    }
    ramdisk_init();
    popeye_plasma_init();

    /* Always boot to terminal. Desktop is launched via 'popeye boot plasma'. */
    terminal_run();

    for (;;)
        __asm__ volatile("hlt");
}
