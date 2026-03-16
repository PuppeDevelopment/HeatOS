/*=============================================================================
 * HeatOS Terminal - 32-bit Protected Mode Shell
 * Full command-line interface running entirely in C (no BIOS calls).
 *===========================================================================*/
#include "terminal.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "io.h"
#include "ramdisk.h"
#include "network.h"
#include "catnake.h"
#include "popeye_plasma.h"
#include "plasma_java.h"
#include "mamu.h"
#include "kat.h"
#include "wget.h"

/* -------------------------------------------------------------------------- */
/* Color theme                                                                 */
/* -------------------------------------------------------------------------- */
#define TERM_NORMAL  VGA_ATTR(VGA_WHITE,       VGA_BLACK)
#define TERM_PROMPT  VGA_ATTR(VGA_WHITE,       VGA_BLACK)
#define TERM_BANNER  VGA_ATTR(VGA_LIGHT_GRAY,  VGA_BLACK)
#define TERM_TITLE   VGA_ATTR(VGA_WHITE,       VGA_BLACK)
#define TERM_INPUT   VGA_ATTR(VGA_WHITE,       VGA_BLACK)

/* -------------------------------------------------------------------------- */
/* Constants                                                                   */
/* -------------------------------------------------------------------------- */
#define INPUT_MAX   79
#define CMD_MAX     15
#define HIST_COUNT   8

/* -------------------------------------------------------------------------- */
/* Terminal state                                                              */
/* -------------------------------------------------------------------------- */
static int cur_row = 0;
static int cur_col = 0;

static char hist_buf[HIST_COUNT][INPUT_MAX + 1];
static int  hist_next = 0;
static int  hist_used = 0;
static char last_run[INPUT_MAX + 1];

static int boot_hour = 0;
static int boot_min  = 0;
static int boot_sec  = 0;

/* ==========================================================================
 * VGA / display helpers
 * ========================================================================== */

static void hw_cursor_sync(void) {
    uint16_t pos = (uint16_t)(cur_row * VGA_WIDTH + cur_col);
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void term_putc(char c, uint8_t attr) {
    if (c == '\n') {
        cur_col = 0;
        cur_row++;
    } else if (c == '\r') {
        cur_col = 0;
    } else {
        vga_putchar_at(cur_row, cur_col, c, attr);
        if (++cur_col >= VGA_WIDTH) { cur_col = 0; cur_row++; }
    }
    if (cur_row >= VGA_HEIGHT) {
        vga_scroll_up(0, VGA_HEIGHT, TERM_NORMAL);
        cur_row = VGA_HEIGHT - 1;
    }
}

static void term_puts_attr(const char *s, uint8_t attr) {
    while (*s) term_putc(*s++, attr);
}

static void term_puts(const char *s)   { term_puts_attr(s, TERM_NORMAL); }
static void term_putln(const char *s)  { term_puts(s); term_putc('\n', TERM_NORMAL); }

static void term_clear_screen(void) {
    vga_clear(TERM_NORMAL);
    cur_row = cur_col = 0;
}

/* numeric printers */
static void print_hex8(uint8_t v) {
    const char *h = "0123456789ABCDEF";
    term_putc(h[(v >> 4) & 0xF], TERM_NORMAL);
    term_putc(h[v & 0xF],        TERM_NORMAL);
}

static void print_hex16(uint16_t v) {
    print_hex8((uint8_t)(v >> 8));
    print_hex8((uint8_t)(v & 0xFF));
}

static void print_dec(uint32_t v) {
    char buf[12];
    itoa((int)v, buf, 10);
    term_puts(buf);
}

static void print_pad2(int n) {
    if (n < 10) term_putc('0', TERM_NORMAL);
    print_dec((uint32_t)n);
}

static void print_ipv4(const uint8_t ip[4]) {
    print_dec((uint32_t)ip[0]); term_putc('.', TERM_NORMAL);
    print_dec((uint32_t)ip[1]); term_putc('.', TERM_NORMAL);
    print_dec((uint32_t)ip[2]); term_putc('.', TERM_NORMAL);
    print_dec((uint32_t)ip[3]);
}

static void print_mac(const uint8_t mac[6]) {
    for (int i = 0; i < 6; i++) {
        if (i) term_putc(':', TERM_NORMAL);
        print_hex8(mac[i]);
    }
}

/* ==========================================================================
 * History helpers
 * ========================================================================== */

static void hist_push(const char *s) {
    strcpy(hist_buf[hist_next], s);
    hist_next = (hist_next + 1) % HIST_COUNT;
    if (hist_used < HIST_COUNT) hist_used++;
}

/* n=1 → most recent, n=hist_used → oldest */
static const char *hist_get(int n) {
    if (n < 1 || n > hist_used) return (void *)0;
    return hist_buf[(hist_next - n + HIST_COUNT * 2) % HIST_COUNT];
}

/* ==========================================================================
 * readline  (backspace, ESC to clear, Up/Down history)
 * ========================================================================== */

static void erase_line(int *len_p, char *buf) {
    while (*len_p > 0) {
        (*len_p)--;
        buf[*len_p] = '\0';
        if (--cur_col < 0) {
            if (cur_row > 0) { cur_col = VGA_WIDTH - 1; cur_row--; }
            else              { cur_col = 0; }
        }
        vga_putchar_at(cur_row, cur_col, ' ', TERM_NORMAL);
    }
    hw_cursor_sync();
}

static void readline(char *buf, int maxlen) {
    int  len  = 0;
    int  hpos = 0;             /* 0 = live buffer */
    char live[INPUT_MAX + 1];
    live[0] = buf[0] = '\0';
    hw_cursor_sync();

    for (;;) {
        int k = 0;
        while (k == 0) {
            network_poll();
            k = keyboard_poll();
        }

        if (k == KEY_ENTER) {
            buf[len] = '\0';
            term_putc('\n', TERM_NORMAL);
            hw_cursor_sync();
            return;
        }

        if (k == KEY_ESCAPE) {
            erase_line(&len, buf);
            live[0] = '\0';
            hpos = 0;
            continue;
        }

        if (k == KEY_BACKSPACE) {
            if (len > 0) {
                len--;
                buf[len] = '\0';
                if (--cur_col < 0) {
                    if (cur_row > 0) { cur_col = VGA_WIDTH - 1; cur_row--; }
                    else              { cur_col = 0; }
                }
                vga_putchar_at(cur_row, cur_col, ' ', TERM_NORMAL);
                hw_cursor_sync();
            }
            continue;
        }

        if (k == KEY_UP) {
            if (hpos == 0) strcpy(live, buf);
            if (hpos < hist_used) {
                hpos++;
                const char *h = hist_get(hpos);
                if (h) {
                    erase_line(&len, buf);
                    strcpy(buf, h);
                    len = (int)strlen(h);
                    term_puts_attr(buf, TERM_INPUT);
                    hw_cursor_sync();
                }
            }
            continue;
        }

        if (k == KEY_DOWN) {
            if (hpos > 0) {
                hpos--;
                erase_line(&len, buf);
                const char *src = (hpos == 0) ? live : hist_get(hpos);
                if (src && src[0]) {
                    strcpy(buf, src);
                    len = (int)strlen(buf);
                    term_puts_attr(buf, TERM_INPUT);
                } else {
                    buf[0] = '\0';
                    len = 0;
                }
                hw_cursor_sync();
            }
            continue;
        }

        /* printable ASCII */
        if (k >= 0x20 && k < 0x7F && len < maxlen) {
            buf[len++] = (char)k;
            buf[len]   = '\0';
            term_putc((char)k, TERM_INPUT);
            hw_cursor_sync();
        }
    }
}

/* ==========================================================================
 * Command parsing
 * ========================================================================== */

static void parse_cmd(const char *in, char *cmd_out, const char **args_out) {
    while (*in == ' ') in++;
    int i = 0;
    while (*in && *in != ' ' && i < CMD_MAX) {
        char c = *in++;
        cmd_out[i++] = (char)((c >= 'A' && c <= 'Z') ? c + 32 : c);
    }
    cmd_out[i] = '\0';
    while (*in == ' ') in++;
    *args_out = in;
}

/* ==========================================================================
 * Hardware helpers (CMOS RTC, PCI)
 * ========================================================================== */

static uint8_t cmos_rd(uint8_t reg) { outb(0x70, reg); return inb(0x71); }
static int bcd(uint8_t v) { return (v >> 4) * 10 + (v & 0xF); }

static void read_rtc(int *h, int *m, int *s, int *day, int *mon, int *year) {
    /* Some emulators/chipsets can hold UIP unexpectedly; avoid boot hang. */
    uint32_t guard = 200000u;
    while ((cmos_rd(0x0A) & 0x80) && guard--) {}
    if (guard == 0u) {
        *h = 0; *m = 0; *s = 0;
        *day = 1; *mon = 1; *year = 2000;
        return;
    }

    *s   = bcd(cmos_rd(0x00));
    *m   = bcd(cmos_rd(0x02));
    *h   = bcd(cmos_rd(0x04));
    *day = bcd(cmos_rd(0x07));
    *mon = bcd(cmos_rd(0x08));
    int y = bcd(cmos_rd(0x09));
    int c = bcd(cmos_rd(0x32));
    if (c == 0) c = 20;
    *year = c * 100 + y;
}

/* ==========================================================================
 * Command implementations
 * ========================================================================== */

static void banner(void) {
    term_puts_attr("HeatOS Console\n", TERM_TITLE);
    term_puts_attr("========================================\n", TERM_BANNER);
    term_puts_attr("Network-aware shell in protected mode\n\n", TERM_BANNER);
}

static void cmd_help(void) {
    term_puts_attr("Commands:\n", TERM_TITLE);
    term_putln("  help      clear/cls   about       version/ver");
    term_putln("  echo      banner      beep        mem");
    term_putln("  date      time        uptime      boot");
    term_putln("  status    history     repeat      apps");
    term_putln("  catnake   mamu <file>  kat <file>  wget <url>");
    term_putln("  ls        cd          pwd         mkdir");
    term_putln("  net       ping <ipv4|domain>   arch");
    term_putln("  java [status|enable|run <demo>|demos]");
    term_putln("  popeye boot plasma    (launch desktop)");
    term_putln("  halt/shutdown  reboot/restart");
    term_putln("Tip: Up/Down arrows scroll history, Esc clears input.");
    term_putc('\n', TERM_NORMAL);
}

static void cmd_about(void) {
    term_putln("HeatOS kernel handles hardware and low-level services.");
    term_putln("32-bit protected mode, C + NASM hybrid kernel.");
    term_putln("Real-mode bootloader -> A20 -> GDT -> 32-bit PM -> C kernel.");
    term_putc('\n', TERM_NORMAL);
}

static void cmd_version(void) {
    term_putln("HeatOS kernel v0.5");
    term_putln("Features: terminal shell, DNS+ICMP networking, ramdisk filesystem.");
    term_putc('\n', TERM_NORMAL);
}

static void cmd_date(void) {
    int h, m, s, day, mon, year;
    read_rtc(&h, &m, &s, &day, &mon, &year);
    (void)h; (void)m; (void)s;
    term_puts("Date: ");
    print_dec((uint32_t)year); term_putc('-', TERM_NORMAL);
    print_pad2(mon);            term_putc('-', TERM_NORMAL);
    print_pad2(day);
    term_putc('\n', TERM_NORMAL);
}

static void cmd_time(void) {
    int h, m, s, day, mon, year;
    read_rtc(&h, &m, &s, &day, &mon, &year);
    (void)day; (void)mon; (void)year;
    term_puts("Time: ");
    print_pad2(h); term_putc(':', TERM_NORMAL);
    print_pad2(m); term_putc(':', TERM_NORMAL);
    print_pad2(s);
    term_putc('\n', TERM_NORMAL);
}

static void cmd_uptime(void) {
    int h, m, s, day, mon, year;
    read_rtc(&h, &m, &s, &day, &mon, &year);
    (void)day; (void)mon; (void)year;
    int now_s  = h * 3600 + m * 60 + s;
    int boot_s = boot_hour * 3600 + boot_min * 60 + boot_sec;
    int elapsed = now_s - boot_s;
    if (elapsed < 0) elapsed += 86400;
    term_puts("Uptime: ");
    print_dec((uint32_t)(elapsed / 3600));       term_putc('h', TERM_NORMAL); term_putc(' ', TERM_NORMAL);
    print_dec((uint32_t)((elapsed % 3600) / 60)); term_putc('m', TERM_NORMAL); term_putc(' ', TERM_NORMAL);
    print_dec((uint32_t)(elapsed % 60));          term_puts("s\n");
}

static void cmd_mem(void) {
    /* CMOS 0x30/0x31: extended memory above 1 MB, in KB */
    uint16_t ext = (uint16_t)cmos_rd(0x30) | ((uint16_t)cmos_rd(0x31) << 8);
    term_puts("Conventional memory: "); print_dec(640);                   term_puts(" KB\n");
    term_puts("Extended memory:     "); print_dec((uint32_t)ext + 1024u); term_puts(" KB\n");
    term_puts("Total (approx):      "); print_dec(640u + 1024u + (uint32_t)ext); term_puts(" KB\n");
}

static void cmd_boot(void) {
    term_puts("Boot: floppy (drive 0x00), 32-bit protected mode\n");
}

static void cmd_status(void) {
    int h, m, s, day, mon, year;
    read_rtc(&h, &m, &s, &day, &mon, &year);
    int now_s  = h * 3600 + m * 60 + s;
    int boot_s = boot_hour * 3600 + boot_min * 60 + boot_sec;
    int elapsed = now_s - boot_s;
    if (elapsed < 0) elapsed += 86400;
    uint16_t ext = (uint16_t)cmos_rd(0x30) | ((uint16_t)cmos_rd(0x31) << 8);

    term_puts_attr("System status:\n", TERM_TITLE);
    term_puts("  version: HeatOS kernel v0.5\n");

    term_puts("  date:    ");
    print_dec((uint32_t)year); term_putc('-', TERM_NORMAL);
    print_pad2(mon);           term_putc('-', TERM_NORMAL);
    print_pad2(day);           term_putc('\n', TERM_NORMAL);

    term_puts("  time:    ");
    print_pad2(h); term_putc(':', TERM_NORMAL);
    print_pad2(m); term_putc(':', TERM_NORMAL);
    print_pad2(s); term_putc('\n', TERM_NORMAL);

    term_puts("  uptime:  ");
    print_dec((uint32_t)(elapsed / 3600));        term_putc('h', TERM_NORMAL); term_putc(' ', TERM_NORMAL);
    print_dec((uint32_t)((elapsed % 3600) / 60)); term_putc('m', TERM_NORMAL); term_putc(' ', TERM_NORMAL);
    print_dec((uint32_t)(elapsed % 60));          term_puts("s\n");

    term_puts("  memory:  ");
    print_dec(640u + 1024u + (uint32_t)ext); term_puts(" KB\n");

    term_puts("  history: ");
    print_dec((uint32_t)hist_used); term_puts(" stored\n\n");
}

static void cmd_history(void) {
    if (!hist_used) { term_putln("No history yet."); return; }
    term_puts_attr("Command history:\n", TERM_TITLE);
    int oldest = (hist_used < HIST_COUNT) ? 0 : hist_next;
    for (int i = 0; i < hist_used; i++) {
        int idx = (oldest + i) % HIST_COUNT;
        print_dec((uint32_t)(i + 1));
        term_puts(". ");
        term_putln(hist_buf[idx]);
    }
    term_putc('\n', TERM_NORMAL);
}

static void cmd_beep(void) {
    uint32_t div = 1193180u / 800u;
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(div & 0xFF));
    outb(0x42, (uint8_t)((div >> 8) & 0xFF));
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 3);
    for (volatile uint32_t i = 0; i < 3000000u; i++) ; /* ~100 ms busy delay */
    outb(0x61, tmp & (uint8_t)~3u);
    term_putln("Beep sent.");
}

static void cmd_arch(void) {
    term_puts_attr("Architecture:\n", TERM_TITLE);
    term_putln("  kernel: 32-bit protected mode, C + NASM");
    term_putln("  boot:   16-bit BIOS -> A20 -> GDT -> 32-bit PM");
    term_putln("  vga:    direct 0xB8000 text-mode writes");
    term_putln("  kbd:    PS/2 port-IO polling (0x60/0x64)");
    term_putln("  rtc:    CMOS port-IO (0x70/0x71)");
    term_putln("  pci:    config-space port-IO (0xCF8/0xCFC)");
    term_putc('\n', TERM_NORMAL);
}

static void cmd_catnake(void) {
    term_clear_screen();
    catnake_run();
    term_clear_screen();
    banner();
    term_puts_attr("Terminal ready.\n\n", TERM_PROMPT);
}

static void cmd_plasma(const char *args) {
    while (*args == ' ') args++;

    if (*args == '\0' || strcmp(args, "status") == 0) {
        term_puts_attr("Popeye Plasma:\n", TERM_TITLE);
        term_puts("  name:      "); term_putln(popeye_plasma_name());
        term_puts("  module:    "); term_putln(popeye_plasma_is_ready() ? "initialized" : "offline");
        term_puts("  boot:      "); term_putln(popeye_plasma_boot_mode());
        term_puts("  release:   ");
        term_putln(popeye_plasma_desktop_boot_allowed() ? "desktop-ready" : "terminal-lock");
        term_puts("  java:      "); term_putln(popeye_plasma_java_support_mode());
        term_putc('\n', TERM_NORMAL);
        return;
    }

    if (strcmp(args, "boot terminal") == 0) {
        popeye_plasma_set_boot_desktop(false);
        term_putln("Popeye Plasma next boot: terminal-first.");
        return;
    }

    if (strcmp(args, "boot desktop") == 0) {
        popeye_plasma_set_boot_desktop(true);
        term_putln("Popeye Plasma next boot: desktop-first.");
        return;
    }

    if (strcmp(args, "start") == 0) {
        term_putln("Launching Popeye Plasma...");
        (void)popeye_plasma_run();
        term_clear_screen();
        banner();
        term_puts_attr("Returned from Popeye Plasma.\n\n", TERM_PROMPT);
        return;
    }

    term_putln("Usage: plasma [status|boot terminal|boot desktop|start]");
}

static void cmd_java(const char *args) {
    while (*args == ' ') args++;

    if (*args == '\0' || strcmp(args, "status") == 0) {
        term_puts_attr("Java Support:\n", TERM_TITLE);
        term_puts("  enabled: ");
        term_putln(popeye_plasma_java_support_enabled() ? "yes" : "no");
        term_puts("  mode:    "); term_putln(popeye_plasma_java_support_mode());
        term_puts("  runtime: HeatOS Mini JVM\n");
        term_puts("  demos:   ");
        for (int i = 0; i < java_vm_demo_count(); i++) {
            if (i) term_puts(", ");
            term_puts(java_vm_demo_name(i));
        }
        term_putc('\n', TERM_NORMAL);
        term_putc('\n', TERM_NORMAL);
        return;
    }

    if (strcmp(args, "enable") == 0) {
        (void)popeye_plasma_enable_java_support();
        java_vm_init();
        term_putln("Java support enabled. Bytecode VM ready.");
        return;
    }

    if (strncmp(args, "run", 3) == 0) {
        const char *name = args + 3;
        while (*name == ' ') name++;
        if (!*name) name = "hello";

        java_result_t result;
        term_puts("Running Java demo: "); term_putln(name);
        term_putc('\n', TERM_NORMAL);

        if (java_vm_run(name, &result)) {
            term_puts(result.output);
        } else {
            term_puts("Error: "); term_putln(result.error);
        }
        term_putc('\n', TERM_NORMAL);
        return;
    }

    if (strcmp(args, "demos") == 0) {
        term_puts_attr("Available Java demos:\n", TERM_TITLE);
        for (int i = 0; i < java_vm_demo_count(); i++) {
            term_puts("  "); term_putln(java_vm_demo_name(i));
        }
        term_putc('\n', TERM_NORMAL);
        return;
    }

    term_putln("Usage: java [status|enable|run <demo>|demos]");
}

static void cmd_popeye(const char *args) {
    while (*args == ' ') args++;

    if (strcmp(args, "boot plasma") == 0) {
        term_putln("Launching Popeye Plasma Desktop...");
        (void)popeye_plasma_run();
        term_clear_screen();
        banner();
        term_puts_attr("Returned from Popeye Plasma.\n\n", TERM_PROMPT);
        return;
    }

    if (*args == '\0' || strcmp(args, "status") == 0) {
        term_puts_attr("Popeye Plasma:\n", TERM_TITLE);
        term_puts("  name:      "); term_putln(popeye_plasma_name());
        term_puts("  module:    "); term_putln(popeye_plasma_is_ready() ? "ready" : "offline");
        term_puts("  java:      "); term_putln(popeye_plasma_java_support_mode());
        term_putc('\n', TERM_NORMAL);
        term_putln("  Launch desktop: popeye boot plasma");
        term_putc('\n', TERM_NORMAL);
        return;
    }

    term_putln("Usage: popeye [boot plasma|status]");
}

static void cmd_mamu(const char *args) {
    while (*args == ' ') args++;
    term_clear_screen();
    mamu_run(args);
    term_clear_screen();
    banner();
    term_puts_attr("Terminal ready.\n\n", TERM_PROMPT);
}

static void cmd_kat(const char *args) {
    while (*args == ' ') args++;
    term_hooks_t hooks = { term_putc, term_puts, term_putln };
    kat_run(args, &hooks);
}

static void cmd_wget(const char *args) {
    while (*args == ' ') args++;
    term_hooks_t hooks = { term_putc, term_puts, term_putln };
    wget_run(args, &hooks);
}

static void cmd_apps(void) {
    term_puts_attr("Terminal commands:\n", TERM_TITLE);
    term_putln("  help, clear, about, version, echo, banner, beep");
    term_putln("  status, history, net, ping <ipv4|domain>, arch");
    term_putln("  ls, cd, pwd, mkdir, java [status|enable|run <demo>|demos]");
    term_putln("  popeye boot plasma    catnake, mamu, kat, wget");
    term_putln("  halt, shutdown, reboot, restart");
    term_putc('\n', TERM_NORMAL);
}

static void cmd_net(void) {
    bool up = network_init();
    net_info_t info;
    network_get_info(&info);

    term_puts_attr("Network:\n", TERM_TITLE);
    term_puts("  state:   ");
    if (!info.present) {
        term_putln("not detected");
        term_putln("  hint:    run QEMU with -nic user,model=ne2k_pci");
        term_putc('\n', TERM_NORMAL);
        return;
    }

    term_putln((up && info.ready) ? "online" : "detected (init failed)");
    term_puts("  slot:    "); print_dec((uint32_t)info.pci_slot); term_putc('\n', TERM_NORMAL);
    term_puts("  vendor:  "); print_hex16(info.vendor_id);         term_putc('\n', TERM_NORMAL);
    term_puts("  device:  "); print_hex16(info.device_id);         term_putc('\n', TERM_NORMAL);
    term_puts("  io-base: 0x"); print_hex16(info.io_base);         term_putc('\n', TERM_NORMAL);

    if (info.ready) {
        term_puts("  mac:     "); print_mac(info.mac);          term_putc('\n', TERM_NORMAL);
        term_puts("  ip:      "); print_ipv4(info.ip);          term_putc('\n', TERM_NORMAL);
        term_puts("  gateway: "); print_ipv4(info.gateway);     term_putc('\n', TERM_NORMAL);
        term_puts("  netmask: "); print_ipv4(info.netmask);     term_putc('\n', TERM_NORMAL);
        term_putln("  dns:     10.0.2.3");
    } else {
        term_putln("  warning: NIC found but driver init failed");
    }
    term_putc('\n', TERM_NORMAL);
}

static void cmd_ping(const char *args) {
    char target[96];
    uint8_t ip[4];
    uint8_t tmp_ip[4];
    bool target_is_ipv4;
    bool used_dns = false;
    net_dns_result_t dns;
    int sent = 0;
    int received = 0;
    uint32_t min_ms = 0;
    uint32_t max_ms = 0;
    uint32_t sum_ms = 0;

    while (*args == ' ') args++;
    if (!*args) { term_putln("Usage: ping <ipv4|domain>"); return; }

    int n = 0;
    while (args[n] && args[n] != ' ' && n < (int)sizeof(target) - 1) {
        target[n] = args[n];
        n++;
    }
    target[n] = '\0';

    target_is_ipv4 = network_parse_ipv4(target, tmp_ip);

    if (!network_init()) {
        term_putln("ping: network unavailable (no ne2k_pci adapter)");
        return;
    }

    if (!network_dns_resolve_a(target, 700000u, ip, &dns)) {
        term_puts_attr("Net Probe\n", TERM_TITLE);
        term_puts_attr("----------------------------------------\n", TERM_BANNER);
        term_puts("target : "); term_putln(target);
        term_putln("resolve: failed");
        term_putln("through: no");
        term_putln("result : FAIL\n");
        return;
    }

    used_dns = !target_is_ipv4 && strcmp(target, "localhost") != 0;

    term_puts_attr("Net Probe\n", TERM_TITLE);
    term_puts_attr("----------------------------------------\n", TERM_BANNER);
    term_puts("target : "); term_putln(target);
    term_puts("ip     : "); print_ipv4(ip); term_putc('\n', TERM_NORMAL);
    if (used_dns) {
        term_puts("resolve: ok (");
        print_dec(dns.time_ms);
        term_putln(" ms)");
    } else {
        term_putln("resolve: direct");
    }
    term_putc('\n', TERM_NORMAL);

    for (int i = 0; i < 4; i++) {
        net_ping_result_t res;
        bool ok;
        sent++;

        ok = network_ping_ipv4(ip, 600000u, &res);

        if (ok && res.success) {
            uint32_t shown_ms = res.time_ms;
            if (shown_ms == 0) shown_ms = 1;

            received++;
            if (received == 1 || shown_ms < min_ms) min_ms = shown_ms;
            if (shown_ms > max_ms) max_ms = shown_ms;
            sum_ms += shown_ms;

            term_puts("[");
            print_dec((uint32_t)(i + 1));
            term_puts("] pass  ");
            print_ipv4(ip);
            term_puts("  rtt=");
            print_dec(shown_ms);
            term_puts("ms  ttl=");
            print_dec((uint32_t)(res.ttl ? res.ttl : 64u));
            term_puts("  bytes=");
            print_dec((uint32_t)(res.bytes ? res.bytes : 32u));
            term_putc('\n', TERM_NORMAL);
        } else {
            term_puts("[");
            print_dec((uint32_t)(i + 1));
            term_putln("] fail  timeout");
        }
    }

    int lost = sent - received;
    int loss_pct = sent ? (lost * 100) / sent : 0;

    term_putc('\n', TERM_NORMAL);
    term_puts("packets: sent=");
    print_dec((uint32_t)sent);
    term_puts(" recv=");
    print_dec((uint32_t)received);
    term_puts(" lost=");
    print_dec((uint32_t)lost);
    term_puts(" loss=");
    print_dec((uint32_t)loss_pct);
    term_putln("%");

    term_puts("through: ");
    term_putln(received > 0 ? "yes" : "no");

    if (received > 0) {
        term_puts("rtt    : min=");
        print_dec(min_ms);
        term_puts("ms avg=");
        print_dec(sum_ms / (uint32_t)received);
        term_puts("ms max=");
        print_dec(max_ms);
        term_putln("ms");
    }

    term_puts("result : ");
    if (received == sent) {
        term_putln("PASS");
    } else if (received > 0) {
        term_putln("PARTIAL");
    } else {
        term_putln("FAIL");
    }

    term_putc('\n', TERM_NORMAL);
}

static void cmd_ls(void) {
    fs_node_t cwd  = fs_cwd_get();
    fs_node_t iter = 0;
    if (!fs_list_begin(cwd, &iter)) { term_putln("(empty)"); return; }
    fs_node_t child;
    while (fs_list_next(&iter, &child)) {
        term_puts(fs_get_name(child));
        if (fs_is_dir(child)) term_putc('/', TERM_NORMAL);
        term_putc('\n', TERM_NORMAL);
    }
}

static void cmd_cd(const char *args) {
    while (*args == ' ') args++;
    if (!*args) { fs_cwd_set(0); return; }      /* bare 'cd' → root */
    fs_node_t node = fs_resolve(args);
    if (fs_is_dir(node)) {
        fs_cwd_set(node);
    } else {
        term_putln("cd: directory not found");
    }
}

static void cmd_pwd(void) {
    char buf[128];
    fs_build_path(fs_cwd_get(), buf, sizeof(buf));
    term_putln(buf);
}

static void cmd_mkdir(const char *args) {
    while (*args == ' ') args++;
    if (!*args) { term_putln("Usage: mkdir <name>"); return; }
    if (!fs_mkdir_child(fs_cwd_get(), args)) {
        term_putln("mkdir: failed (already exists or node table full)");
    }
}

static void cmd_halt(void) {
    term_putln("System halted.");
    __asm__ volatile("cli");
    for (;;) __asm__ volatile("hlt");
}

static void cmd_reboot(void) {
    term_putln("Rebooting...");
    while (inb(0x64) & 2) {}   /* wait for keyboard controller input buffer empty */
    outb(0x64, 0xFE);           /* pulse reset line */
    for (;;) {}
}

/* ==========================================================================
 * Main terminal entry point
 * ========================================================================== */

void terminal_run(void) {
    /* Capture boot time for uptime calculation */
    int dummy_day, dummy_mon, dummy_year;
    read_rtc(&boot_hour, &boot_min, &boot_sec, &dummy_day, &dummy_mon, &dummy_year);

    ramdisk_init();
    (void)network_init();
    term_clear_screen();
    banner();
    term_puts_attr("Terminal ready.\n\n", TERM_PROMPT);

    static char input[INPUT_MAX + 1];
    static char cmd[CMD_MAX + 1];
    const char *args;

    for (;;) {
        /* Print prompt with cwd */
        char pwd_buf[64];
        fs_build_path(fs_cwd_get(), pwd_buf, sizeof(pwd_buf));
        term_puts_attr("heatos", TERM_PROMPT);
        term_puts_attr(pwd_buf, TERM_PROMPT);
        term_puts_attr("$ ", TERM_PROMPT);

        readline(input, INPUT_MAX);

        /* Trim leading whitespace */
        const char *p = input;
        while (*p == ' ') p++;
        if (!*p) continue;

        /* Save to history */
        hist_push(p);

        /* Parse command and args */
        parse_cmd(p, cmd, &args);

        /* Handle 'repeat' specially */
        if (strcmp(cmd, "repeat") == 0) {
            if (!last_run[0]) { term_putln("Nothing to repeat yet."); continue; }
            term_puts("Replaying: "); term_putln(last_run);
            parse_cmd(last_run, cmd, &args);
            /* fall through to dispatch the replayed command */
        } else {
            strcpy(last_run, p);
        }

        /* ---- Command dispatch ---- */
        if      (strcmp(cmd, "help")     == 0) cmd_help();
        else if (strcmp(cmd, "clear")    == 0 ||
                 strcmp(cmd, "cls")      == 0) term_clear_screen();
        else if (strcmp(cmd, "about")    == 0) cmd_about();
        else if (strcmp(cmd, "version")  == 0 ||
                 strcmp(cmd, "ver")      == 0) cmd_version();
        else if (strcmp(cmd, "echo")     == 0) {
            if (*args) term_putln(args);
            else       term_putln("Usage: echo <text>");
        }
        else if (strcmp(cmd, "banner")   == 0) banner();
        else if (strcmp(cmd, "beep")     == 0) cmd_beep();
        else if (strcmp(cmd, "date")     == 0) cmd_date();
        else if (strcmp(cmd, "time")     == 0) cmd_time();
        else if (strcmp(cmd, "uptime")   == 0) cmd_uptime();
        else if (strcmp(cmd, "mem")      == 0) cmd_mem();
        else if (strcmp(cmd, "boot")     == 0) cmd_boot();
        else if (strcmp(cmd, "status")   == 0) cmd_status();
        else if (strcmp(cmd, "history")  == 0) cmd_history();
        else if (strcmp(cmd, "net")      == 0) cmd_net();
        else if (strcmp(cmd, "ping")     == 0) cmd_ping(args);
        else if (strcmp(cmd, "arch")     == 0) cmd_arch();
        else if (strcmp(cmd, "plasma")   == 0) cmd_plasma(args);
        else if (strcmp(cmd, "popeye")   == 0) cmd_popeye(args);
        else if (strcmp(cmd, "java")     == 0) cmd_java(args);
        else if (strcmp(cmd, "apps")     == 0) cmd_apps();
        else if (strcmp(cmd, "catnake")  == 0) cmd_catnake();
        else if (strcmp(cmd, "mamu")     == 0) cmd_mamu(args);
        else if (strcmp(cmd, "kat")      == 0) cmd_kat(args);
        else if (strcmp(cmd, "wget")     == 0) cmd_wget(args);
        else if (strcmp(cmd, "ls")       == 0) cmd_ls();
        else if (strcmp(cmd, "cd")       == 0) cmd_cd(args);
        else if (strcmp(cmd, "pwd")      == 0) cmd_pwd();
        else if (strcmp(cmd, "mkdir")    == 0) cmd_mkdir(args);
        else if (strcmp(cmd, "halt")     == 0 ||
                 strcmp(cmd, "shutdown") == 0) cmd_halt();
        else if (strcmp(cmd, "reboot")   == 0 ||
                 strcmp(cmd, "restart")  == 0) cmd_reboot();
        else {
            term_puts("Unknown command: "); term_puts(cmd);
            term_putln(". Type 'help'.");
        }
    }
}
