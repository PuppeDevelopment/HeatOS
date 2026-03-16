#include "kat.h"
#include "ramdisk.h"
#include "terminal.h"
#include "types.h"
#include "vga.h"

void kat_run(const char *filename) {
    if (!filename || !*filename) {
        term_putln("Usage: kat <file>");
        return;
    }
    
    fs_node_t node = fs_resolve(filename);
    if (!node || !fs_is_file(node)) {
        term_putln("kat: file not found");
        return;
    }

    char buf[512];
    int n = fs_read(node, buf, sizeof(buf) - 1);
    if (n >= 0) {
        buf[n] = '\0';
        term_puts(buf);
        if (n > 0 && buf[n-1] != '\n') term_putc('\n', VGA_ATTR(VGA_WHITE, VGA_BLACK));
    } else {
        term_putln("kat: error reading file");
    }
}
