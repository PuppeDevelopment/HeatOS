// Minimal 64-bit kernel entry used during migration.
// This intentionally avoids the 32-bit subsystem stack until ported.

static void vga_write_line(const char *s, int row, unsigned char attr) {
    volatile unsigned short *vga = (volatile unsigned short *)0xB8000;
    int col = 0;

    while (s[col] && col < 80) {
        vga[row * 80 + col] = (unsigned short)attr << 8 | (unsigned char)s[col];
        col++;
    }
}

void kernel_main64(void) {
    volatile unsigned short *vga = (volatile unsigned short *)0xB8000;

    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = (unsigned short)0x07 << 8 | ' ';
    }

    vga_write_line("HeatOS x64 bootstrap", 2, 0x0F);
    vga_write_line("Long mode active (phase 1)", 4, 0x0A);
    vga_write_line("Next: port IDT, MM, scheduler, terminal", 6, 0x07);

    for (;;) {
        __asm__ volatile("hlt");
    }
}
