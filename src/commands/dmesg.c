#include "terminal.h"
#include "klog.h"
#include "string.h"

#define DMESG_PRINT_BUFFER 4096

void cmd_dmesg(const char *args) {
    static char dump[DMESG_PRINT_BUFFER];
    bool clear_after = false;

    if (args) {
        while (*args == ' ') args++;
        if (strcmp(args, "-c") == 0) {
            clear_after = true;
        }
    }

    int n = klog_read(dump, DMESG_PRINT_BUFFER);
    if (n <= 0) {
        term_puts("dmesg: log buffer is empty\n");
    } else {
        term_puts(dump);
        if (klog_bytes_used() >= (DMESG_PRINT_BUFFER - 1)) {
            term_puts("dmesg: output truncated, use a larger buffer in kernel build\n");
        }
    }

    if (clear_after) {
        klog_clear();
        term_puts("dmesg: buffer cleared\n");
    }
}
