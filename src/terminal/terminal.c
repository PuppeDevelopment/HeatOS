#include "terminal.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "commands.h"

static int cursor_x = 0;
static int cursor_y = 0;

static void str_to_lower_in_place(char *s) {
    if (!s) return;
    while (*s) {
        if (*s >= 'A' && *s <= 'Z') {
            *s = (char)(*s - 'A' + 'a');
        }
        s++;
    }
}

void term_putc(char c, uint8_t attr) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_putchar_at(cursor_y, cursor_x, ' ', attr);
        }
    } else {
        vga_putchar_at(cursor_y, cursor_x, c, attr);
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll_up(0, VGA_HEIGHT - 1, attr);
        cursor_y = VGA_HEIGHT - 1;
    }
    vga_set_cursor(cursor_y, cursor_x);
}

void term_puts(const char *s) {
    while (*s) {
        term_putc(*s++, 0x07);
    }
}

void term_reset_screen(void) {
    vga_clear(0x07);
    cursor_x = 0;
    cursor_y = 0;
    vga_set_cursor(cursor_y, cursor_x);
}

void terminal_run(void) {
    term_reset_screen();
    term_puts("Welcome to HeatOS!\n");
    term_puts("The whole kernel have been rewritten for stability.\n> ");
    
    char cmdbuf[256];
    int cmdlen = 0;
    
    while (1) {
        int key = keyboard_poll();
        if (key) {
            if (key == KEY_ENTER || key == '\n') {
                term_putc('\n', 0x07);
                cmdbuf[cmdlen] = '\0';
                
                if (cmdlen > 0) {
                    char *args = strchr(cmdbuf, ' ');
                    if (args) {
                        *args = '\0';
                        args++;
                        while (*args == ' ') args++; // Skip multiple spaces
                        if (*args == '\0') args = NULL;
                    }

                    str_to_lower_in_place(cmdbuf);

                    if (strcmp(cmdbuf, "help") == 0) {
                        term_puts("Available commands:\n");
                        term_puts("  File:    ls mkdir rm touch cat cd pwd tree cp find grep rmdir mv\n");
                        term_puts("  Network: ping wget nslookup ifconfig netstat traceroute curl telnet\n");
                        term_puts("  System:  echo uname whoami date ps kill top dmesg kstats reboot shutdown\n");
                        term_puts("  Games:   catnake pong\n");
                        term_puts("  Apps:    mamu kat desktop\n");
                        term_puts("  Note:    echo supports > redirection\n");
                        term_puts("  UI:      help clear\n");
                    } else if (strcmp(cmdbuf, "clear") == 0) {
                        term_reset_screen();
                    } else if (cmd_dispatch(cmdbuf, args)) {
                        // Command found and handled.
                    } else {
                        term_puts("Unknown command: ");
                        term_puts(cmdbuf);
                        term_puts("\n");
                    }
                }
                cmdlen = 0;
                term_puts("> ");
            } else if (key == KEY_BACKSPACE || key == '\b') {
                if (cmdlen > 0) {
                    cmdlen--;
                    term_putc('\b', 0x07);
                }
            } else if (key >= 32 && key <= 126 && cmdlen < 255) {
                cmdbuf[cmdlen++] = key;
                term_putc((char)key, 0x07);
            }
        }
    }
}
