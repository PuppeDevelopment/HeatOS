/*=============================================================================
 * Popeye Plasma Desktop Environment - Complete Rewrite
 * Modern text-mode desktop with KDE Plasma-inspired design.
 * Uses CP437 box-drawing characters and a dark color theme.
 *===========================================================================*/
extern "C" {
#include "vga.h"
#include "keyboard.h"
#include "mouse.h"
#include "network.h"
#include "string.h"
#include "io.h"
#include "plasma_java.h"
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Color Theme - Dark Modern Palette                                          */
/* ═══════════════════════════════════════════════════════════════════════════ */

/* Desktop */
#define PP_BG          VGA_ATTR(VGA_DARK_GRAY, VGA_BLUE)
#define PP_BG_ACCENT   VGA_ATTR(VGA_BLUE, VGA_BLUE)

/* Bottom panel (taskbar) */
#define PP_PANEL       VGA_ATTR(VGA_WHITE, VGA_BLACK)
#define PP_PANEL_DIM   VGA_ATTR(VGA_DARK_GRAY, VGA_BLACK)
#define PP_PANEL_CLK   VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK)
#define PP_START       VGA_ATTR(VGA_WHITE, VGA_CYAN)
#define PP_START_ACT   VGA_ATTR(VGA_WHITE, VGA_LIGHT_CYAN)
#define PP_TAB_ACT     VGA_ATTR(VGA_WHITE, VGA_DARK_GRAY)
#define PP_TAB_INACT   VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK)

/* Window */
#define PP_TITLE       VGA_ATTR(VGA_WHITE, VGA_CYAN)
#define PP_BORDER      VGA_ATTR(VGA_LIGHT_CYAN, VGA_DARK_GRAY)
#define PP_BODY        VGA_ATTR(VGA_WHITE, VGA_DARK_GRAY)
#define PP_BODY_DIM    VGA_ATTR(VGA_LIGHT_GRAY, VGA_DARK_GRAY)
#define PP_BODY_HL     VGA_ATTR(VGA_LIGHT_CYAN, VGA_DARK_GRAY)
#define PP_BODY_VAL    VGA_ATTR(VGA_YELLOW, VGA_DARK_GRAY)
#define PP_BODY_OK     VGA_ATTR(VGA_LIGHT_GREEN, VGA_DARK_GRAY)
#define PP_BODY_ERR    VGA_ATTR(VGA_LIGHT_RED, VGA_DARK_GRAY)
#define PP_CLOSE       VGA_ATTR(VGA_WHITE, VGA_RED)

/* Start menu */
#define PP_MENU        VGA_ATTR(VGA_WHITE, VGA_BLACK)
#define PP_MENU_SEL    VGA_ATTR(VGA_WHITE, VGA_CYAN)
#define PP_MENU_HDR    VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK)
#define PP_MENU_BORDER VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK)

/* Desktop icons */
#define PP_ICON_FILES  VGA_ATTR(VGA_YELLOW, VGA_BLUE)
#define PP_ICON_TERM   VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLUE)
#define PP_ICON_NET    VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLUE)
#define PP_ICON_JAVA   VGA_ATTR(VGA_LIGHT_RED, VGA_BLUE)
#define PP_ICON_LBL    VGA_ATTR(VGA_WHITE, VGA_BLUE)

/* System tray */
#define PP_NET_UP      VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK)
#define PP_NET_DOWN    VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK)
#define PP_JAVA_ON     VGA_ATTR(VGA_YELLOW, VGA_BLACK)

/* ═══════════════════════════════════════════════════════════════════════════ */
/* CP437 Box-Drawing Characters                                               */
/* ═══════════════════════════════════════════════════════════════════════════ */
#define CH_DTL   '\xC9'   /* ╔ */
#define CH_DTR   '\xBB'   /* ╗ */
#define CH_DBL   '\xC8'   /* ╚ */
#define CH_DBR   '\xBC'   /* ╝ */
#define CH_DH    '\xCD'   /* ═ */
#define CH_DV    '\xBA'   /* ║ */
#define CH_DLT   '\xCC'   /* ╠ */
#define CH_DRT   '\xB9'   /* ╣ */
#define CH_SV    '\xB3'   /* │ */
#define CH_SHADE '\xB0'   /* ░ */
#define CH_BLOCK '\xDB'   /* █ */
#define CH_LHBLK '\xDC'   /* ▄ */
#define CH_UHBLK '\xDF'   /* ▀ */
#define CH_BULLET '\xFE'  /* ■ */
#define CH_ARROW '\x10'   /* ► */
#define CH_DOT   '\xF9'   /* ∙ */

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Application IDs                                                            */
/* ═══════════════════════════════════════════════════════════════════════════ */
enum AppId {
    APP_NONE     = -1,
    APP_HOME     =  0,
    APP_FILES    =  1,
    APP_NET      =  2,
    APP_JAVA     =  3,
    APP_TERMINAL =  4,
    APP_SHUTDOWN =  5,
};

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Layout Constants                                                           */
/* ═══════════════════════════════════════════════════════════════════════════ */
#define WIN_TOP     2
#define WIN_LEFT    12
#define WIN_WIDTH   66
#define WIN_HEIGHT  21
#define WIN_RIGHT   (WIN_LEFT + WIN_WIDTH - 1)
#define WIN_BOTTOM  (WIN_TOP + WIN_HEIGHT - 1)

#define TASKBAR_ROW 24
#define MENU_WIDTH  26
#define MENU_ITEM_COUNT 6

/* ═══════════════════════════════════════════════════════════════════════════ */
/* State                                                                      */
/* ═══════════════════════════════════════════════════════════════════════════ */
static AppId g_app      = APP_NONE;
static bool  g_menu_open = false;
static int   g_menu_sel  = 0;
static int   g_mouse_x   = 40;
static int   g_mouse_y   = 12;
static bool  g_mouse_ldown = false;
static bool  g_mouse_ok  = false;
static bool  g_exit      = false;
static bool  g_java_on   = false;

/* Java VM state */
static java_result_t g_java_result;
static bool g_java_ran   = false;
static int  g_java_demo  = 0;

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Helpers                                                                    */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void put(int r, int c, const char *s, uint8_t attr) {
    while (*s && c < VGA_WIDTH) {
        vga_putchar_at(r, c++, *s++, attr);
    }
}

static void putch(int r, int c, char ch, uint8_t attr) {
    vga_putchar_at(r, c, ch, attr);
}

static void fill(int r, int c, int h, int w, char ch, uint8_t attr) {
    vga_fill_rect(r, c, h, w, ch, attr);
}

static int slen(const char *s) { return (int)strlen(s); }

static void clamp_mouse(void) {
    if (g_mouse_x < 0) g_mouse_x = 0;
    if (g_mouse_x >= VGA_WIDTH) g_mouse_x = VGA_WIDTH - 1;
    if (g_mouse_y < 0) g_mouse_y = 0;
    if (g_mouse_y >= VGA_HEIGHT) g_mouse_y = VGA_HEIGHT - 1;
}

/* CMOS RTC */
static uint8_t cmos_rd(uint8_t reg) { outb(0x70, reg); return inb(0x71); }
static int bcd2(uint8_t v) { return (v >> 4) * 10 + (v & 0xF); }
static void get_time(int *h, int *m) {
    uint32_t guard = 100000u;
    while ((cmos_rd(0x0A) & 0x80) && guard--) {}
    *h = bcd2(cmos_rd(0x04));
    *m = bcd2(cmos_rd(0x02));
}

static void pad2(int v, char *buf) {
    buf[0] = '0' + v / 10;
    buf[1] = '0' + v % 10;
    buf[2] = '\0';
}

static void format_ip(const uint8_t ip[4], char *out) {
    int p = 0; char part[12];
    for (int i = 0; i < 4; i++) {
        utoa((uint32_t)ip[i], part, 10);
        for (int j = 0; part[j] && p < 20; j++) out[p++] = part[j];
        if (i < 3) out[p++] = '.';
    }
    out[p] = '\0';
}

static bool hit(int x, int y, int left, int top, int w, int h) {
    return x >= left && x < left + w && y >= top && y < top + h;
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Drawing: Double-line Box                                                   */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void draw_dbox(int top, int left, int h, int w, uint8_t border, uint8_t body) {
    int bot = top + h - 1;
    int right = left + w - 1;
    fill(top, left, h, w, ' ', body);
    putch(top, left, CH_DTL, border);
    putch(top, right, CH_DTR, border);
    putch(bot, left, CH_DBL, border);
    putch(bot, right, CH_DBR, border);
    for (int x = left + 1; x < right; x++) {
        putch(top, x, CH_DH, border);
        putch(bot, x, CH_DH, border);
    }
    for (int y = top + 1; y < bot; y++) {
        putch(y, left, CH_DV, border);
        putch(y, right, CH_DV, border);
    }
}

static void draw_dsep(int row, int left, int w, uint8_t attr) {
    putch(row, left, CH_DLT, attr);
    putch(row, left + w - 1, CH_DRT, attr);
    for (int x = left + 1; x < left + w - 1; x++)
        putch(row, x, CH_DH, attr);
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Desktop Wallpaper                                                          */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void draw_wallpaper(void) {
    for (int r = 0; r < VGA_HEIGHT - 1; r++) {
        for (int c = 0; c < VGA_WIDTH; c++) {
            /* Subtle dot-grid pattern on dark blue */
            char ch = ((r + c) % 5 == 0) ? CH_DOT : ' ';
            putch(r, c, ch, PP_BG);
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Desktop Icons                                                              */
/* ═══════════════════════════════════════════════════════════════════════════ */
struct DesktopIcon {
    int row, col;
    const char *label;
    AppId app;
    uint8_t color;
};

static DesktopIcon g_icons[] = {
    {  2, 2, "Files",   APP_FILES,    PP_ICON_FILES },
    {  6, 2, "Term",    APP_TERMINAL, PP_ICON_TERM  },
    { 10, 2, "Network", APP_NET,      PP_ICON_NET   },
    { 14, 2, "Java",    APP_JAVA,     PP_ICON_JAVA  },
};
#define ICON_COUNT 4

static void draw_icons(void) {
    for (int i = 0; i < ICON_COUNT; i++) {
        DesktopIcon &ic = g_icons[i];
        /* 2x2 colored block icon */
        putch(ic.row,     ic.col,     CH_BLOCK, ic.color);
        putch(ic.row,     ic.col + 1, CH_BLOCK, ic.color);
        putch(ic.row + 1, ic.col,     CH_BLOCK, ic.color);
        putch(ic.row + 1, ic.col + 1, CH_BLOCK, ic.color);
        /* Label */
        put(ic.row + 2, ic.col, ic.label, PP_ICON_LBL);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Taskbar                                                                    */
/* ═══════════════════════════════════════════════════════════════════════════ */
struct TabBtn {
    const char *label;
    AppId app;
    int col, width;
};

static TabBtn g_tabs[] = {
    { "Home",  APP_HOME,  0, 0 },
    { "Files", APP_FILES, 0, 0 },
    { "Net",   APP_NET,   0, 0 },
    { "Java",  APP_JAVA,  0, 0 },
};
#define TAB_COUNT 4

static void draw_taskbar(void) {
    fill(TASKBAR_ROW, 0, 1, VGA_WIDTH, ' ', PP_PANEL);

    /* Start button */
    uint8_t start_attr = g_menu_open ? PP_START_ACT : PP_START;
    putch(TASKBAR_ROW, 1, CH_BULLET, start_attr);
    put(TASKBAR_ROW, 2, " Start ", start_attr);

    /* Separator */
    putch(TASKBAR_ROW, 10, CH_SV, PP_PANEL_DIM);

    /* Tab buttons */
    int col = 12;
    for (int i = 0; i < TAB_COUNT; i++) {
        g_tabs[i].col = col;
        bool active = (g_app == g_tabs[i].app);
        uint8_t attr = active ? PP_TAB_ACT : PP_TAB_INACT;
        int w = slen(g_tabs[i].label) + 2;
        g_tabs[i].width = w;
        putch(TASKBAR_ROW, col, ' ', attr);
        put(TASKBAR_ROW, col + 1, g_tabs[i].label, attr);
        putch(TASKBAR_ROW, col + w - 1, ' ', attr);
        col += w + 1;
    }

    /* System tray: Network */
    net_info_t info;
    network_get_info(&info);
    if (info.present && info.ready) {
        put(TASKBAR_ROW, 60, " Net", PP_NET_UP);
        putch(TASKBAR_ROW, 64, '\x18', PP_NET_UP);
    } else {
        put(TASKBAR_ROW, 60, " Net", PP_NET_DOWN);
        putch(TASKBAR_ROW, 64, '\x19', PP_NET_DOWN);
    }

    /* System tray: Java */
    if (g_java_on)
        put(TASKBAR_ROW, 66, " J", PP_JAVA_ON);

    /* Separator + Clock */
    putch(TASKBAR_ROW, 69, CH_SV, PP_PANEL_DIM);
    int h, m;
    get_time(&h, &m);
    char hh[3], mm[3];
    pad2(h, hh); pad2(m, mm);
    put(TASKBAR_ROW, 71, hh, PP_PANEL_CLK);
    putch(TASKBAR_ROW, 73, ':', PP_PANEL_CLK);
    put(TASKBAR_ROW, 74, mm, PP_PANEL_CLK);
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Start Menu                                                                 */
/* ═══════════════════════════════════════════════════════════════════════════ */
struct MenuItem { const char *label; AppId app; };

static MenuItem g_menu_items[] = {
    { "Home",      APP_HOME     },
    { "Files",     APP_FILES    },
    { "Network",   APP_NET      },
    { "Java",      APP_JAVA     },
    { "Terminal",  APP_TERMINAL },
    { "Shut Down", APP_SHUTDOWN },
};

static void draw_menu(void) {
    if (!g_menu_open) return;

    int menu_h = MENU_ITEM_COUNT + 4;
    int menu_top = TASKBAR_ROW - menu_h;

    draw_dbox(menu_top, 0, menu_h, MENU_WIDTH, PP_MENU_BORDER, PP_MENU);

    /* Header */
    put(menu_top + 1, 2, " Popeye Plasma", PP_MENU_HDR);
    draw_dsep(menu_top + 2, 0, MENU_WIDTH, PP_MENU_BORDER);

    /* Items */
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        int row = menu_top + 3 + i;
        bool sel = (i == g_menu_sel);
        uint8_t attr = sel ? PP_MENU_SEL : PP_MENU;
        fill(row, 1, 1, MENU_WIDTH - 2, ' ', attr);
        putch(row, 2, sel ? CH_ARROW : ' ', attr);
        put(row, 4, g_menu_items[i].label, attr);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Window Frame                                                               */
/* ═══════════════════════════════════════════════════════════════════════════ */
static const char *app_title(AppId app) {
    switch (app) {
        case APP_HOME:  return " Home ";
        case APP_FILES: return " Files ";
        case APP_NET:   return " Network ";
        case APP_JAVA:  return " Java ";
        default:        return " Window ";
    }
}

static void draw_window(void) {
    if (g_app < APP_HOME || g_app > APP_JAVA) return;

    /* Body fill + border */
    draw_dbox(WIN_TOP, WIN_LEFT, WIN_HEIGHT, WIN_WIDTH, PP_BORDER, PP_BODY);

    /* Title bar: overwrite top row with accent */
    for (int x = WIN_LEFT; x <= WIN_RIGHT; x++)
        putch(WIN_TOP, x, CH_DH, PP_TITLE);
    putch(WIN_TOP, WIN_LEFT, CH_DTL, PP_TITLE);
    putch(WIN_TOP, WIN_RIGHT, CH_DTR, PP_TITLE);

    /* Title text centered */
    const char *title = app_title(g_app);
    int tlen = slen(title);
    int tx = WIN_LEFT + (WIN_WIDTH - tlen) / 2;
    put(WIN_TOP, tx, title, PP_TITLE);

    /* Close [X] button */
    putch(WIN_TOP, WIN_RIGHT - 3, '[', PP_TITLE);
    putch(WIN_TOP, WIN_RIGHT - 2, 'X', PP_CLOSE);
    putch(WIN_TOP, WIN_RIGHT - 1, ']', PP_TITLE);
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* App Content Renderers                                                      */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void render_home(void) {
    int r = WIN_TOP + 2, c = WIN_LEFT + 3;

    put(r, c, "Welcome to Popeye Plasma", PP_BODY_HL); r += 2;
    put(r, c, "Desktop Environment for HeatOS", PP_BODY); r += 2;

    put(r, c, "Navigation:", PP_BODY_HL); r++;
    put(r, c + 2, "Click taskbar buttons or desktop icons", PP_BODY_DIM); r++;
    put(r, c + 2, "Tab / M : toggle start menu", PP_BODY_DIM); r++;
    put(r, c + 2, "1-4     : switch apps directly", PP_BODY_DIM); r++;
    put(r, c + 2, "Esc     : close window", PP_BODY_DIM); r++;
    put(r, c + 2, "F2 / T  : exit to terminal", PP_BODY_DIM); r += 2;

    put(r, c, "System:", PP_BODY_HL); r++;
    put(r, c + 2, "Kernel   HeatOS v0.5", PP_BODY); r++;
    put(r, c + 2, "Desktop  Popeye Plasma", PP_BODY); r++;
    put(r, c + 2, "Arch     i386 Protected Mode", PP_BODY); r++;
    put(r, c + 2, "Java     ", PP_BODY);
    put(r, c + 11, g_java_on ? "active" : "disabled", g_java_on ? PP_BODY_OK : PP_BODY_DIM);
}

static void render_files(void) {
    int r = WIN_TOP + 2, c = WIN_LEFT + 3;

    put(r, c, "File System (ramdisk)", PP_BODY_HL); r += 2;

    const char *dirs[] = { "/apps", "/docs", "/home", "/java", "/plasma", "/system" };
    for (int i = 0; i < 6; i++) {
        putch(r, c, CH_BULLET, PP_BODY_VAL);
        put(r, c + 2, dirs[i], PP_BODY); r++;
    }
    r++;
    put(r, c, "Use terminal for full commands: ls, cd, pwd, mkdir", PP_BODY_DIM);
}

static void render_net(void) {
    int r = WIN_TOP + 2, c = WIN_LEFT + 3;
    net_info_t info;
    network_get_info(&info);

    put(r, c, "Network Status", PP_BODY_HL); r += 2;

    if (!info.present) {
        put(r, c, "Adapter:  ", PP_BODY);
        put(r, c + 10, "Not detected", PP_BODY_ERR); r += 2;
        put(r, c, "Hint: qemu -nic user,model=ne2k_pci", PP_BODY_DIM);
        return;
    }

    char ip[24], gw[24], nm[24], slot_buf[12];
    format_ip(info.ip, ip);
    format_ip(info.gateway, gw);
    format_ip(info.netmask, nm);
    utoa((uint32_t)info.pci_slot, slot_buf, 10);

    put(r, c, "Adapter:  ", PP_BODY);
    put(r, c + 10, "NE2000 PCI", PP_BODY_VAL); r++;
    put(r, c, "State:    ", PP_BODY);
    put(r, c + 10, info.ready ? "Online" : "Detected", info.ready ? PP_BODY_OK : PP_BODY_ERR); r++;
    put(r, c, "Slot:     ", PP_BODY);
    put(r, c + 10, slot_buf, PP_BODY_VAL); r += 2;

    if (info.ready) {
        put(r, c, "IP:       ", PP_BODY);
        put(r, c + 10, ip, PP_BODY_VAL); r++;
        put(r, c, "Gateway:  ", PP_BODY);
        put(r, c + 10, gw, PP_BODY_VAL); r++;
        put(r, c, "Netmask:  ", PP_BODY);
        put(r, c + 10, nm, PP_BODY_VAL); r++;
        put(r, c, "DNS:      ", PP_BODY);
        put(r, c + 10, "10.0.2.3", PP_BODY_VAL);
    }
}

static void render_java(void) {
    int r = WIN_TOP + 2, c = WIN_LEFT + 3;

    put(r, c, "Java Bytecode VM", PP_BODY_HL); r += 2;

    put(r, c, "Status:   ", PP_BODY);
    put(r, c + 10, g_java_on ? "Enabled" : "Disabled", g_java_on ? PP_BODY_OK : PP_BODY_DIM); r++;
    put(r, c, "Runtime:  ", PP_BODY);
    put(r, c + 10, "HeatOS Mini JVM", PP_BODY_VAL); r += 2;

    /* Demo selector */
    int demo_count = java_vm_demo_count();
    put(r, c, "Demo:     ", PP_BODY);
    if (demo_count > 0)
        put(r, c + 10, java_vm_demo_name(g_java_demo), PP_BODY_VAL);
    r++;
    put(r, c, "  N: next demo   Enter/R: run", PP_BODY_DIM); r += 2;

    /* Output area */
    if (g_java_ran) {
        if (g_java_result.success) {
            put(r, c, "Output:", PP_BODY_HL); r++;
            /* Render output line by line */
            const char *p = g_java_result.output;
            while (*p && r < WIN_BOTTOM - 1) {
                int len = 0;
                while (p[len] && p[len] != '\n' && len < WIN_WIDTH - 8) len++;
                /* Print this line */
                for (int i = 0; i < len; i++)
                    putch(r, c + 2 + i, p[i], PP_BODY_OK);
                r++;
                p += len;
                if (*p == '\n') p++;
            }
        } else {
            put(r, c, "Error: ", PP_BODY_ERR);
            put(r, c + 7, g_java_result.error, PP_BODY_ERR);
        }
    }
}

static void render_app_content(void) {
    switch (g_app) {
        case APP_HOME:  render_home(); break;
        case APP_FILES: render_files(); break;
        case APP_NET:   render_net(); break;
        case APP_JAVA:  render_java(); break;
        default: break;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Mouse Cursor (inverted cell)                                               */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void draw_cursor(void) {
    uint16_t cell = vga_read_at(g_mouse_y, g_mouse_x);
    uint8_t ch   = (uint8_t)(cell & 0xFF);
    uint8_t attr = (uint8_t)(cell >> 8);
    uint8_t fg = attr & 0x0F;
    uint8_t bg = (attr >> 4) & 0x0F;
    uint8_t inv = VGA_ATTR(bg, fg);
    if (ch == ' ' || ch == 0) ch = CH_BLOCK;
    vga_putchar_at(g_mouse_y, g_mouse_x, ch, inv);
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Input: App Activation                                                      */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void activate_app(AppId app) {
    if (app == APP_TERMINAL) {
        g_exit = true;
        g_menu_open = false;
        return;
    }
    if (app == APP_SHUTDOWN) {
        /* Halt the system */
        vga_clear(VGA_ATTR(VGA_WHITE, VGA_BLACK));
        vga_write_at(12, 30, "System halted.", VGA_ATTR(VGA_WHITE, VGA_BLACK));
        __asm__ volatile("cli");
        for (;;) __asm__ volatile("hlt");
    }
    g_app = app;
    g_menu_open = false;
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Input: Mouse Click                                                         */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void handle_click(void) {
    int mx = g_mouse_x, my = g_mouse_y;

    /* Taskbar: Start button */
    if (my == TASKBAR_ROW && hit(mx, my, 1, TASKBAR_ROW, 9, 1)) {
        g_menu_open = !g_menu_open;
        return;
    }

    /* Taskbar: Tab buttons */
    if (my == TASKBAR_ROW) {
        for (int i = 0; i < TAB_COUNT; i++) {
            if (hit(mx, my, g_tabs[i].col, TASKBAR_ROW, g_tabs[i].width, 1)) {
                activate_app(g_tabs[i].app);
                return;
            }
        }
    }

    /* Start menu items */
    if (g_menu_open) {
        int menu_h = MENU_ITEM_COUNT + 4;
        int menu_top = TASKBAR_ROW - menu_h;
        for (int i = 0; i < MENU_ITEM_COUNT; i++) {
            if (hit(mx, my, 1, menu_top + 3 + i, MENU_WIDTH - 2, 1)) {
                g_menu_sel = i;
                activate_app(g_menu_items[i].app);
                return;
            }
        }
        /* Click outside menu closes it */
        g_menu_open = false;
        return;
    }

    /* Window close button [X] */
    if (g_app >= APP_HOME && g_app <= APP_JAVA) {
        if (hit(mx, my, WIN_RIGHT - 3, WIN_TOP, 3, 1)) {
            g_app = APP_NONE;
            return;
        }
    }

    /* Desktop icons */
    for (int i = 0; i < ICON_COUNT; i++) {
        if (hit(mx, my, g_icons[i].col, g_icons[i].row, 8, 3)) {
            activate_app(g_icons[i].app);
            return;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Input: Keyboard                                                            */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void handle_key(int key) {
    /* Menu navigation */
    if (g_menu_open) {
        if (key == KEY_UP)   { g_menu_sel = (g_menu_sel + MENU_ITEM_COUNT - 1) % MENU_ITEM_COUNT; return; }
        if (key == KEY_DOWN) { g_menu_sel = (g_menu_sel + 1) % MENU_ITEM_COUNT; return; }
        if (key == KEY_ENTER) { activate_app(g_menu_items[g_menu_sel].app); return; }
        if (key == KEY_ESCAPE) { g_menu_open = false; return; }
    }

    /* Global shortcuts */
    if (key == KEY_TAB || key == 'm' || key == 'M') { g_menu_open = !g_menu_open; return; }
    if (key == KEY_F2 || key == 't' || key == 'T')   { g_exit = true; return; }
    if (key == KEY_ESCAPE) {
        if (g_app != APP_NONE) g_app = APP_NONE;
        else g_exit = true;
        return;
    }

    /* Number shortcuts to open apps */
    if (key == '1') { activate_app(APP_HOME);  return; }
    if (key == '2') { activate_app(APP_FILES); return; }
    if (key == '3') { activate_app(APP_NET);   return; }
    if (key == '4') { activate_app(APP_JAVA);  return; }

    /* Java app controls */
    if (g_app == APP_JAVA) {
        if (key == 'n' || key == 'N') {
            g_java_demo = (g_java_demo + 1) % java_vm_demo_count();
            g_java_ran = false;
            return;
        }
        if (key == 'r' || key == 'R' || key == KEY_ENTER) {
            java_vm_run(java_vm_demo_name(g_java_demo), &g_java_result);
            g_java_ran = true;
            return;
        }
    }

    /* Enter opens home if no window */
    if (key == KEY_ENTER && g_app == APP_NONE) {
        activate_app(APP_HOME);
        return;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Render Full Frame                                                          */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void render_frame(void) {
    draw_wallpaper();
    draw_icons();
    if (g_app >= APP_HOME && g_app <= APP_JAVA) {
        draw_window();
        render_app_content();
    }
    draw_taskbar();
    draw_menu();
    draw_cursor();
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Main Desktop Loop                                                          */
/* ═══════════════════════════════════════════════════════════════════════════ */
bool popeye_plasma_desktop_run(bool java_enabled) {
    g_app        = APP_NONE;
    g_menu_open  = false;
    g_menu_sel   = 0;
    g_mouse_x    = VGA_WIDTH / 2;
    g_mouse_y    = VGA_HEIGHT / 2;
    g_mouse_ldown = false;
    g_exit       = false;
    g_java_on    = java_enabled;
    g_java_ran   = false;
    g_java_demo  = 0;

    g_mouse_ok = mouse_init();
    if (g_mouse_ok)
        mouse_set_reporting(true);

    (void)network_init();
    java_vm_init();

    while (!g_exit) {
        network_poll();

        /* Mouse input */
        if (g_mouse_ok) {
            mouse_packet_t pkt;
            bool left_now = g_mouse_ldown;
            while (mouse_poll(&pkt)) {
                g_mouse_x += (int)pkt.dx;
                g_mouse_y -= (int)pkt.dy;
                clamp_mouse();
                left_now = pkt.left;
            }
            if (!g_mouse_ldown && left_now)
                handle_click();
            g_mouse_ldown = left_now;
        }

        /* Keyboard input */
        int key = keyboard_poll();
        if (key) handle_key(key);

        render_frame();
    }

    if (g_mouse_ok)
        mouse_set_reporting(false);

    return true;
}
