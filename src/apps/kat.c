#include "kat.h"
#include "ramdisk.h"
#include "types.h"
#include "vga.h"

void kat_run(const char *filename, term_hooks_t *hooks) {
    if (!hooks) return;

    if (!filename || !*filename) {
        hooks->putln("Usage: kat <file>");
        return;
    }
    
    fs_node_t node = fs_resolve(filename);
    if (!node || !fs_is_file(node)) {
        hooks->putln("kat: file not found");
        return;
    }

    int n;
    
    /* 
       Note: Our current fs_read implementation always returns the FULL file data 
       starting from offset 0 (it doesn't track a seek pointer).
       To be truly efficient for large files, we'd need a seek-aware fs_read.
       For now, let's just read the whole thing in one go if it fits in a reasonable buffer.
    */
    
    char large_buf[RAMDISK_DATA_CAP + 1];
    n = fs_read(node, large_buf, RAMDISK_DATA_CAP);
    if (n >= 0) {
        large_buf[n] = '\0';
        hooks->puts(large_buf);
        if (n > 0 && large_buf[n-1] != '\n') {
            hooks->putc('\n', VGA_ATTR(VGA_WHITE, VGA_BLACK));
        }
    } else {
        hooks->putln("kat: error reading file");
    }
}
