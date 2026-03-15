/*=============================================================================
 * RushOS Applications — All built-in apps for the Popeye Plasma desktop
 *===========================================================================*/
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "io.h"
#include "desktop.h"

/* Defined in desktop.c */
extern void draw_app_frame(const char *title);

/* ---- RTC helpers ---- */
static uint8_t rtc_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd2bin(uint8_t v) {
    return (v >> 4) * 10 + (v & 0x0F);
}

/* ---- PCI helpers ---- */
static uint32_t pci_cfg_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off) {
    outl(0xCF8, (1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)slot << 11)
                | ((uint32_t)func << 8) | (off & 0xFC));
    return inl(0xCFC);
}

/*=============================================================================
 * Terminal App — "Rush>" shell with built-in commands
 *===========================================================================*/
#define TERM_TOP     2
#define TERM_BOTTOM 23
#define TERM_ATTR   THEME_TERMINAL
#define PROMPT      "Rush> "
#define MAX_CMD     64

static int term_row;
static int term_col;

static void term_scroll(void) {
    vga_scroll_up(TERM_TOP, TERM_BOTTOM + 1, TERM_ATTR);
    term_row = TERM_BOTTOM;
    term_col = 0;
}

static void term_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
        if (term_row > TERM_BOTTOM) term_scroll();
        return;
    }
    if (term_col >= VGA_WIDTH) {
        term_col = 0;
        term_row++;
        if (term_row > TERM_BOTTOM) term_scroll();
    }
    vga_putchar_at(term_row, term_col, c, TERM_ATTR);
    term_col++;
}

static void term_print(const char *s) {
    while (*s) term_putchar(*s++);
}

static void term_println(const char *s) {
    term_print(s);
    term_putchar('\n');
}

/* ---- Terminal commands ---- */
static void cmd_help(void) {
    term_println("Available commands:");
    term_println("  help       - Show this help");
    term_println("  clear      - Clear screen");
    term_println("  sysinfo    - System information");
    term_println("  time       - Current time");
    term_println("  date       - Current date");
    term_println("  net        - Network adapter info");
    term_println("  pci        - List PCI devices");
    term_println("  echo <msg> - Print message");
    term_println("  reboot     - Reboot system");
    term_println("  exit       - Return to desktop");
    term_println("  arch       - Architecture info");
    term_println("  uptime     - System uptime");
    term_println("  popeye     - About desktop env");
}

static void cmd_sysinfo(void) {
    term_println("RushOS x86 32-bit Protected Mode");
    term_println("Kernel: C + NASM hybrid");
    term_println("Desktop: Popeye Plasma 2.0");
    term_println("Arch: i386 (32-bit protected mode)");

    char buf[16];
    /* Detect RAM via CMOS (base memory, registers 0x15/0x16) */
    uint8_t lo = rtc_read(0x15);
    uint8_t hi = rtc_read(0x16);
    uint32_t base_kb = ((uint32_t)hi << 8) | lo;
    term_print("Base memory: ");
    utoa(base_kb, buf, 10);
    term_print(buf);
    term_println(" KB");
}

static void cmd_time(void) {
    char buf[16];
    uint8_t h = bcd2bin(rtc_read(0x04));
    uint8_t m = bcd2bin(rtc_read(0x02));
    uint8_t s = bcd2bin(rtc_read(0x00));
    buf[0] = '0' + h / 10; buf[1] = '0' + h % 10; buf[2] = ':';
    buf[3] = '0' + m / 10; buf[4] = '0' + m % 10; buf[5] = ':';
    buf[6] = '0' + s / 10; buf[7] = '0' + s % 10; buf[8] = '\0';
    term_print("Time: ");
    term_println(buf);
}

static void cmd_date(void) {
    char buf[16];
    uint8_t y = bcd2bin(rtc_read(0x09));
    uint8_t mon = bcd2bin(rtc_read(0x08));
    uint8_t d = bcd2bin(rtc_read(0x07));
    term_print("Date: 20");
    buf[0] = '0' + y / 10; buf[1] = '0' + y % 10;
    buf[2] = '-';
    buf[3] = '0' + mon / 10; buf[4] = '0' + mon % 10;
    buf[5] = '-';
    buf[6] = '0' + d / 10; buf[7] = '0' + d % 10;
    buf[8] = '\0';
    term_println(buf);
}

static void cmd_net(void) {
    /* Scan PCI for network class (0x02) */
    bool found = false;
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t id = pci_cfg_read(0, slot, 0, 0);
        if ((id & 0xFFFF) == 0xFFFF) continue;

        uint32_t cls = pci_cfg_read(0, slot, 0, 0x08);
        uint8_t base_class = (cls >> 24) & 0xFF;
        if (base_class != 0x02) continue;

        found = true;
        char buf[16];
        uint16_t vendor = id & 0xFFFF;
        uint16_t device = (id >> 16) & 0xFFFF;

        term_print("NIC found at slot ");
        utoa(slot, buf, 10);
        term_println(buf);
        term_print("  Vendor: 0x");
        utoa(vendor, buf, 16);
        term_println(buf);
        term_print("  Device: 0x");
        utoa(device, buf, 16);
        term_println(buf);
    }
    if (!found)
        term_println("No network adapter detected.");
}

static void cmd_pci(void) {
    char buf[16];
    term_println("PCI Bus Scan (bus 0):");
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t id = pci_cfg_read(0, slot, 0, 0);
        if ((id & 0xFFFF) == 0xFFFF) continue;

        uint32_t cls = pci_cfg_read(0, slot, 0, 0x08);
        uint8_t base_class = (cls >> 24) & 0xFF;
        uint8_t sub_class = (cls >> 16) & 0xFF;

        term_print("  Slot ");
        utoa(slot, buf, 10);
        term_print(buf);
        term_print(": Vendor=0x");
        utoa(id & 0xFFFF, buf, 16);
        term_print(buf);
        term_print(" Device=0x");
        utoa((id >> 16) & 0xFFFF, buf, 16);
        term_print(buf);
        term_print(" Class=");
        utoa(base_class, buf, 16);
        term_print(buf);
        term_print(":");
        utoa(sub_class, buf, 16);
        term_println(buf);
    }
}

static void cmd_arch(void) {
    term_println("Architecture: x86 (i386)");
    term_println("Mode: 32-bit Protected Mode");
    term_println("Segments: Flat model (0-4GB)");
    term_println("Boot: BIOS -> Real Mode -> PM");
    term_println("VGA: Text mode 80x25, direct memory 0xB8000");
    term_println("Input: PS/2 keyboard polling (port 0x60/0x64)");
}

static void cmd_popeye(void) {
    term_println("=== Popeye Plasma Desktop Environment ===");
    term_println("Version: 2.0 (C rewrite)");
    term_println("Style: KDE Plasma-inspired text mode");
    term_println("Features:");
    term_println("  - Kickoff menu (M key)");
    term_println("  - Run dialog (R key)");
    term_println("  - Quick terminal (F2)");
    term_println("  - 7 built-in applications");
    term_println("  - Full keyboard navigation");
}

static void dispatch_command(const char *cmd) {
    if (strcmp(cmd, "help") == 0)    { cmd_help(); return; }
    if (strcmp(cmd, "sysinfo") == 0) { cmd_sysinfo(); return; }
    if (strcmp(cmd, "time") == 0)    { cmd_time(); return; }
    if (strcmp(cmd, "date") == 0)    { cmd_date(); return; }
    if (strcmp(cmd, "net") == 0)     { cmd_net(); return; }
    if (strcmp(cmd, "pci") == 0)     { cmd_pci(); return; }
    if (strcmp(cmd, "arch") == 0)    { cmd_arch(); return; }
    if (strcmp(cmd, "popeye") == 0 || strcmp(cmd, "plasma") == 0) { cmd_popeye(); return; }
    if (strcmp(cmd, "uptime") == 0)  { term_println("Uptime tracking not yet impl."); return; }
    if (strcmp(cmd, "clear") == 0) {
        vga_fill_rect(TERM_TOP, 0, TERM_BOTTOM - TERM_TOP + 1, VGA_WIDTH, ' ', TERM_ATTR);
        term_row = TERM_TOP;
        term_col = 0;
        return;
    }
    if (strncmp(cmd, "echo ", 5) == 0) { term_println(cmd + 5); return; }
    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "desktop") == 0 || strcmp(cmd, "gui") == 0) return;
    if (strcmp(cmd, "reboot") == 0) {
        /* Triple fault reboot: load null IDT and trigger interrupt */
        uint8_t null_idt[6] = {0};
        __asm__ volatile("lidt %0; int $3" : : "m"(null_idt));
        return;
    }

    if (strlen(cmd) > 0) {
        term_print("Unknown command: ");
        term_println(cmd);
        term_println("Type 'help' for available commands.");
    }
}

void app_terminal(void) {
    draw_app_frame("Terminal - Rush Shell");
    vga_fill_rect(TERM_TOP, 0, TERM_BOTTOM - TERM_TOP + 1, VGA_WIDTH, ' ', TERM_ATTR);

    term_row = TERM_TOP;
    term_col = 0;
    term_println("RushOS Terminal v2.0 (32-bit Protected Mode)");
    term_println("Type 'help' for commands. 'exit' to return.");
    term_putchar('\n');

    char cmd[MAX_CMD];
    int pos;

    for (;;) {
        term_print(PROMPT);
        pos = 0;
        memset(cmd, 0, MAX_CMD);

        for (;;) {
            int key = keyboard_wait();
            if (key == KEY_ESCAPE) return;
            if (key == KEY_ENTER) {
                cmd[pos] = '\0';
                term_putchar('\n');
                break;
            }
            if (key == KEY_BACKSPACE && pos > 0) {
                pos--;
                term_col--;
                vga_putchar_at(term_row, term_col, ' ', TERM_ATTR);
                continue;
            }
            if (key >= 32 && key < 127 && pos < MAX_CMD - 1) {
                cmd[pos++] = (char)key;
                term_putchar((char)key);
            }
        }

        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "desktop") == 0 || strcmp(cmd, "gui") == 0)
            return;

        dispatch_command(cmd);
    }
}

/*=============================================================================
 * System Info App
 *===========================================================================*/
void app_system(void) {
    draw_app_frame("System Information");
    char buf[16];

    int row = 3;
    vga_write_at(row++, 4, "=== RushOS System Information ===", THEME_TITLE);
    row++;
    vga_write_at(row++, 4, "OS:        RushOS", THEME_APP_BG);
    vga_write_at(row++, 4, "Kernel:    C + NASM hybrid", THEME_APP_BG);
    vga_write_at(row++, 4, "Mode:      32-bit Protected Mode", THEME_APP_BG);
    vga_write_at(row++, 4, "Desktop:   Popeye Plasma 2.0", THEME_APP_BG);
    vga_write_at(row++, 4, "Video:     VGA Text 80x25", THEME_APP_BG);
    vga_write_at(row++, 4, "Input:     PS/2 Keyboard (polling)", THEME_APP_BG);
    row++;

    /* Base memory from CMOS */
    uint8_t lo = rtc_read(0x15);
    uint8_t hi = rtc_read(0x16);
    uint32_t base_kb = ((uint32_t)hi << 8) | lo;
    vga_write_at(row, 4, "Base RAM:  ", THEME_APP_BG);
    utoa(base_kb, buf, 10);
    vga_write_at(row, 15, buf, THEME_APP_BG);
    vga_write_at(row, 15 + (int)strlen(buf), " KB", THEME_APP_BG);
    row += 2;

    /* Extended memory from CMOS (registers 0x17/0x18) */
    lo = rtc_read(0x17);
    hi = rtc_read(0x18);
    uint32_t ext_kb = ((uint32_t)hi << 8) | lo;
    vga_write_at(row, 4, "Ext RAM:   ", THEME_APP_BG);
    utoa(ext_kb, buf, 10);
    vga_write_at(row, 15, buf, THEME_APP_BG);
    vga_write_at(row, 15 + (int)strlen(buf), " KB", THEME_APP_BG);

    while (keyboard_wait() != KEY_ESCAPE)
        ;
}

/*=============================================================================
 * Files App (placeholder)
 *===========================================================================*/
void app_files(void) {
    draw_app_frame("File Browser");
    vga_write_at(5, 10, "No filesystem implemented yet.", THEME_APP_BG);
    vga_write_at(7, 10, "RushOS runs from a raw floppy image.", THEME_APP_BG);
    vga_write_at(8, 10, "A FAT12 driver is planned.", THEME_APP_BG);

    while (keyboard_wait() != KEY_ESCAPE)
        ;
}

/*=============================================================================
 * Notes App
 *===========================================================================*/
void app_notes(void) {
    draw_app_frame("Notes - RushOS Roadmap");

    int row = 3;
    vga_write_at(row++, 4, "=== RushOS Development Roadmap ===", THEME_TITLE);
    row++;
    vga_write_at(row++, 4, "[x] 32-bit Protected Mode kernel in C", THEME_APP_BG);
    vga_write_at(row++, 4, "[x] Modular file structure", THEME_APP_BG);
    vga_write_at(row++, 4, "[x] Popeye Plasma 2.0 desktop", THEME_APP_BG);
    vga_write_at(row++, 4, "[x] Terminal with shell commands", THEME_APP_BG);
    vga_write_at(row++, 4, "[x] PCI bus scanning", THEME_APP_BG);
    vga_write_at(row++, 4, "[x] RTC clock", THEME_APP_BG);
    vga_write_at(row++, 4, "[ ] PS/2 Mouse driver", THEME_APP_BG);
    vga_write_at(row++, 4, "[ ] FAT12 filesystem", THEME_APP_BG);
    vga_write_at(row++, 4, "[ ] IRQ-driven keyboard", THEME_APP_BG);
    vga_write_at(row++, 4, "[ ] VGA graphics mode", THEME_APP_BG);
    vga_write_at(row++, 4, "[ ] NIC packet TX/RX", THEME_APP_BG);
    vga_write_at(row++, 4, "[ ] Memory management", THEME_APP_BG);
    vga_write_at(row++, 4, "[ ] Process scheduler", THEME_APP_BG);

    while (keyboard_wait() != KEY_ESCAPE)
        ;
}

/*=============================================================================
 * Clock App
 *===========================================================================*/
void app_clock(void) {
    draw_app_frame("Clock");
    char buf[16];

    vga_write_at(4, 10, "=== RTC Clock ===", THEME_TITLE);
    vga_write_at(6, 10, "Press any key to refresh, Esc to exit.", THEME_APP_BG);

    for (;;) {
        uint8_t h = bcd2bin(rtc_read(0x04));
        uint8_t m = bcd2bin(rtc_read(0x02));
        uint8_t s = bcd2bin(rtc_read(0x00));
        uint8_t y = bcd2bin(rtc_read(0x09));
        uint8_t mon = bcd2bin(rtc_read(0x08));
        uint8_t d = bcd2bin(rtc_read(0x07));

        /* Time */
        buf[0] = '0' + h / 10; buf[1] = '0' + h % 10; buf[2] = ':';
        buf[3] = '0' + m / 10; buf[4] = '0' + m % 10; buf[5] = ':';
        buf[6] = '0' + s / 10; buf[7] = '0' + s % 10; buf[8] = '\0';
        vga_write_at(9, 15, "Time: ", THEME_APP_BG);
        vga_write_at(9, 21, buf, VGA_ATTR(VGA_YELLOW, VGA_BLUE));

        /* Date */
        char datebuf[16];
        datebuf[0] = '2'; datebuf[1] = '0';
        datebuf[2] = '0' + y / 10; datebuf[3] = '0' + y % 10; datebuf[4] = '-';
        datebuf[5] = '0' + mon / 10; datebuf[6] = '0' + mon % 10; datebuf[7] = '-';
        datebuf[8] = '0' + d / 10; datebuf[9] = '0' + d % 10; datebuf[10] = '\0';
        vga_write_at(11, 15, "Date: ", THEME_APP_BG);
        vga_write_at(11, 21, datebuf, VGA_ATTR(VGA_YELLOW, VGA_BLUE));

        int key = keyboard_wait();
        if (key == KEY_ESCAPE) return;
    }
}

/*=============================================================================
 * Network App
 *===========================================================================*/
void app_network(void) {
    draw_app_frame("Network Information");
    char buf[16];

    int row = 3;
    vga_write_at(row++, 4, "=== PCI Network Adapter Scan ===", THEME_TITLE);
    row++;

    bool found = false;
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t id = pci_cfg_read(0, slot, 0, 0);
        if ((id & 0xFFFF) == 0xFFFF) continue;

        uint32_t cls = pci_cfg_read(0, slot, 0, 0x08);
        uint8_t base_class = (cls >> 24) & 0xFF;
        if (base_class != 0x02) continue;

        found = true;
        uint16_t vendor = id & 0xFFFF;
        uint16_t device = (id >> 16) & 0xFFFF;

        vga_write_at(row++, 6, "Network adapter found!", VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLUE));
        row++;

        vga_write_at(row, 6, "PCI Slot:  ", THEME_APP_BG);
        utoa(slot, buf, 10);
        vga_write_at(row++, 17, buf, THEME_APP_BG);

        vga_write_at(row, 6, "Vendor ID: 0x", THEME_APP_BG);
        utoa(vendor, buf, 16);
        vga_write_at(row++, 19, buf, THEME_APP_BG);

        vga_write_at(row, 6, "Device ID: 0x", THEME_APP_BG);
        utoa(device, buf, 16);
        vga_write_at(row++, 19, buf, THEME_APP_BG);

        uint8_t sub_class = (cls >> 16) & 0xFF;
        vga_write_at(row, 6, "Type:      ", THEME_APP_BG);
        if (sub_class == 0x00) vga_write_at(row, 17, "Ethernet Controller", THEME_APP_BG);
        else vga_write_at(row, 17, "Other Network Controller", THEME_APP_BG);
        row++;

        /* Check for known vendor names */
        row++;
        if (vendor == 0x10EC) vga_write_at(row, 6, "Vendor: Realtek", THEME_APP_BG);
        else if (vendor == 0x8086) vga_write_at(row, 6, "Vendor: Intel", THEME_APP_BG);
        else if (vendor == 0x10DE) vga_write_at(row, 6, "Vendor: NVIDIA", THEME_APP_BG);
        else if (vendor == 0x1022) vga_write_at(row, 6, "Vendor: AMD", THEME_APP_BG);
        else vga_write_at(row, 6, "Vendor: Unknown", THEME_APP_BG);
        break;
    }

    if (!found) {
        vga_write_at(row++, 6, "No network adapter detected.", VGA_ATTR(VGA_LIGHT_RED, VGA_BLUE));
        vga_write_at(row + 1, 6, "Try: qemu -nic user,model=ne2k_pci", THEME_APP_BG);
    }

    while (keyboard_wait() != KEY_ESCAPE)
        ;
}

/*=============================================================================
 * Power App
 *===========================================================================*/
void app_power(void) {
    draw_app_frame("Power Management");

    vga_write_at(5, 15, "=== Power Options ===", THEME_TITLE);
    vga_write_at(8, 15, "[R] Reboot System", THEME_APP_BG);
    vga_write_at(10, 15, "[H] Halt (power off)", THEME_APP_BG);
    vga_write_at(12, 15, "[Esc] Cancel", THEME_APP_BG);

    for (;;) {
        int key = keyboard_wait();
        if (key == KEY_ESCAPE) return;
        if (key == 'r' || key == 'R') {
            vga_write_at(16, 15, "Rebooting...", VGA_ATTR(VGA_YELLOW, VGA_BLUE));
            /* Triple fault reboot */
            uint8_t null_idt[6] = {0};
            __asm__ volatile("lidt %0; int $3" : : "m"(null_idt));
        }
        if (key == 'h' || key == 'H') {
            vga_write_at(16, 15, "Halting...", VGA_ATTR(VGA_YELLOW, VGA_BLUE));
            /* Try ACPI shutdown (QEMU) via port 0x604 */
            outw(0x604, 0x2000);
            /* If that didn't work, just halt */
            __asm__ volatile("cli; hlt");
        }
    }
}
