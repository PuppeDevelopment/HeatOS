#include "terminal.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "commands.h"
#include "pit.h"

/* ---- Display state ---- */
static int cursor_x = 0;
static int cursor_y = 0;

/* ---- Command history (50-entry ring buffer, from VibeOS session 29) ---- */
#define HIST_SIZE 50
#define HIST_LEN  256

static char g_history[HIST_SIZE][HIST_LEN];
static int  g_hist_count = 0;
static int  g_hist_next  = 0;
static char g_pending[HIST_LEN];
static int  g_nav_pos = 0;

/* ---- Line editor state ---- */
static char cmdbuf[HIST_LEN];
static int  cmdlen     = 0;
static int  cursor_pos = 0;
static int  edit_row   = 0;
static int  edit_col   = 2;

/* ============================================================
   History helpers
   ============================================================ */

static void hist_add(const char *cmd) {
    if (!cmd || !*cmd) return;
    strncpy(g_history[g_hist_next], cmd, HIST_LEN - 1);
    g_history[g_hist_next][HIST_LEN - 1] = '\0';
    g_hist_next = (g_hist_next + 1) % HIST_SIZE;
    if (g_hist_count < HIST_SIZE) g_hist_count++;
}

static const char *hist_get(int steps_back) {
    if (steps_back < 1 || steps_back > g_hist_count) return (void*)0;
    int idx = (g_hist_next - steps_back + HIST_SIZE * 2) % HIST_SIZE;
    return g_history[idx];
}

void term_print_history(void) {
    if (g_hist_count == 0) {
        term_puts("  (no history)\n");
        return;
    }
    char nbuf[8];
    for (int i = 1; i <= g_hist_count; i++) {
        const char *entry = hist_get(g_hist_count - i + 1);
        if (!entry) continue;
        itoa(i, nbuf, 10);
        term_puts("  ");
        term_puts(nbuf);
        if (i < 10) term_puts("   ");
        else        term_puts("  ");
        term_puts(entry);
        term_puts("\n");
    }
}

/* ============================================================
   Core output
   ============================================================ */

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
        edit_row = cursor_y;
    }
    vga_set_cursor(cursor_y, cursor_x);
}

void term_puts(const char *s) {
    while (*s) term_putc(*s++, 0x07);
}

void term_reset_screen(void) {
    vga_clear(0x07);
    cursor_x = 0;
    cursor_y = 0;
    vga_set_cursor(cursor_y, cursor_x);
}

/* ============================================================
   Line editor helpers
   ============================================================ */

static void redraw_input(void) {
    int col   = edit_col;
    int row   = edit_row;
    int avail = VGA_WIDTH - col;
    for (int i = 0; i < cmdlen && i < avail; i++)
        vga_putchar_at(row, col + i, cmdbuf[i], 0x07);
    for (int i = cmdlen; i < avail; i++)
        vga_putchar_at(row, col + i, ' ', 0x07);
    cursor_x = col + cursor_pos;
    cursor_y = row;
    vga_set_cursor(cursor_y, cursor_x);
}

static void load_history_entry(int steps_back) {
    const char *entry = hist_get(steps_back);
    if (!entry) return;
    strncpy(cmdbuf, entry, HIST_LEN - 1);
    cmdbuf[HIST_LEN - 1] = '\0';
    cmdlen     = (int)strlen(cmdbuf);
    cursor_pos = cmdlen;
    redraw_input();
}

static void print_prompt(void) {
    term_puts("> ");
    edit_row = cursor_y;
    edit_col = cursor_x;
}

static void str_to_lower_in_place(char *s) {
    if (!s) return;
    while (*s) {
        if (*s >= 'A' && *s <= 'Z') *s = (char)(*s - 'A' + 'a');
        s++;
    }
}

/* ============================================================
   Main shell loop
   ============================================================ */

void terminal_run(void) {
    term_reset_screen();
    term_puts("Welcome to HeatOS!\n");
    term_puts("Type 'help' for commands.  Up/Down=history  Home/End/Del=editing\n");
    print_prompt();

    cmdlen     = 0;
    cursor_pos = 0;
    cmdbuf[0]  = '\0';

    while (1) {
        /* HLT-idle: CPU stays in low-power state until next IRQ. */
        int key = keyboard_poll();
        if (!key) {
            __asm__ volatile("hlt");
            continue;
        }

        if (key == KEY_ENTER || key == '\n') {
            cmdbuf[cmdlen] = '\0';
            term_putc('\n', 0x07);
            g_nav_pos = 0;

            if (cmdlen > 0) {
                hist_add(cmdbuf);

                char tmp[HIST_LEN];
                strncpy(tmp, cmdbuf, HIST_LEN - 1);
                tmp[HIST_LEN - 1] = '\0';

                char *args = strchr(tmp, ' ');
                if (args) {
                    *args = '\0';
                    args++;
                    while (*args == ' ') args++;
                    if (*args == '\0') args = (void*)0;
                }

                str_to_lower_in_place(tmp);

                if (strcmp(tmp, "help") == 0) {
                    term_puts("Available commands:\n");
                    term_puts("  File:    ls mkdir rm touch cat cd pwd tree cp find grep rmdir mv\n");
                    term_puts("  Network: ping wget nslookup ifconfig netstat traceroute curl telnet\n");
                    term_puts("  System:  echo uname whoami date ps kill top dmesg kstats reboot shutdown\n");
                    term_puts("  System+: uptime sleep calc beep history\n");
                    term_puts("  Games:   catnake pong\n");
                    term_puts("  Apps:    mamu kat desktop\n");
                    term_puts("  Tips:    echo > file  |  Up/Down=history  |  Home/End=line ends\n");
                } else if (strcmp(tmp, "clear") == 0) {
                    term_reset_screen();
                } else if (strcmp(tmp, "history") == 0) {
                    term_print_history();
                } else if (cmd_dispatch(tmp, args)) {
                    /* handled */
                } else {
                    term_puts("Unknown command: ");
                    term_puts(tmp);
                    term_puts("  (try 'help')\n");
                }
            }

            cmdlen     = 0;
            cursor_pos = 0;
            cmdbuf[0]  = '\0';
            print_prompt();
            continue;
        }

        if (key == KEY_BACKSPACE || key == '\b') {
            if (cursor_pos > 0) {
                for (int i = cursor_pos - 1; i < cmdlen - 1; i++)
                    cmdbuf[i] = cmdbuf[i + 1];
                cmdlen--;
                cursor_pos--;
                cmdbuf[cmdlen] = '\0';
                redraw_input();
            }
            continue;
        }

        if (key == KEY_DELETE) {
            if (cursor_pos < cmdlen) {
                for (int i = cursor_pos; i < cmdlen - 1; i++)
                    cmdbuf[i] = cmdbuf[i + 1];
                cmdlen--;
                cmdbuf[cmdlen] = '\0';
                redraw_input();
            }
            continue;
        }

        if (key == KEY_LEFT) {
            if (cursor_pos > 0) { cursor_pos--; redraw_input(); }
            continue;
        }
        if (key == KEY_RIGHT) {
            if (cursor_pos < cmdlen) { cursor_pos++; redraw_input(); }
            continue;
        }
        if (key == KEY_HOME) {
            cursor_pos = 0; redraw_input();
            continue;
        }
        if (key == KEY_END) {
            cursor_pos = cmdlen; redraw_input();
            continue;
        }

        if (key == KEY_UP) {
            if (g_nav_pos == 0) {
                strncpy(g_pending, cmdbuf, HIST_LEN - 1);
                g_pending[HIST_LEN - 1] = '\0';
            }
            if (g_nav_pos < g_hist_count) {
                g_nav_pos++;
                load_history_entry(g_nav_pos);
            }
            continue;
        }
        if (key == KEY_DOWN) {
            if (g_nav_pos > 1) {
                g_nav_pos--;
                load_history_entry(g_nav_pos);
            } else if (g_nav_pos == 1) {
                g_nav_pos = 0;
                strncpy(cmdbuf, g_pending, HIST_LEN - 1);
                cmdbuf[HIST_LEN - 1] = '\0';
                cmdlen     = (int)strlen(cmdbuf);
                cursor_pos = cmdlen;
                redraw_input();
            }
            continue;
        }

        if (key >= 32 && key <= 126 && cmdlen < HIST_LEN - 1) {
            for (int i = cmdlen; i > cursor_pos; i--)
                cmdbuf[i] = cmdbuf[i - 1];
            cmdbuf[cursor_pos] = (char)key;
            cmdlen++;
            cursor_pos++;
            cmdbuf[cmdlen] = '\0';
            redraw_input();
        }
    }
}
