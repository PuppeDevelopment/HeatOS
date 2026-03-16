/*=============================================================================
 * Popeye Plasma Desktop Environment - High Resolution BGA Rewrite
 * High-res graphical desktop with KDE Plasma-inspired light theme.
 *===========================================================================*/
extern "C" {
#include "framebuffer.h"
#include "keyboard.h"
#include "mouse.h"
#include "network.h"
#include "string.h"
#include "io.h"
#include "plasma_java.h"
#include "bga.h"
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Color Theme - KDE Plasma Breeze Light Palette                              */
/* ═══════════════════════════════════════════════════════════════════════════ */

/* ARGB / XRGB format: 0x00RRGGBB */
#define CLR_TEXT_DARK   0x001A1A1A  /* Near black text */
#define CLR_TEXT_LIGHT  0x00FFFFFF  /* White text */
#define CLR_TEXT_DIM    0x00606060  /* Gray text */

#define CLR_PANEL       0x00EFEFEF  /* Taskbar background */
#define CLR_PANEL_EDGE  0x00CCCCCC
#define CLR_TAB_ACT     0x00FFFFFF  /* Active taskbar tab */
#define CLR_TAB_INACT   0x00E0E0E0
#define CLR_TAB_HOT     0x00D0EBFF  /* Hover taskbar tab */

#define CLR_WIN_BODY    0x00FFFFFF  /* White window background */
#define CLR_WIN_TITLE   0x003DAEE9  /* Plasma Blue */
#define CLR_WIN_BORDER  0x00B0B0B0  /* Light border */
#define CLR_WIN_SHADOW  0x00404040  /* Fake drop shadow */

#define CLR_MENU        0x00FAFAFA
#define CLR_MENU_SEL    0x003DAEE9
#define CLR_MENU_SEL_T  0x00FFFFFF

#define CLR_OK          0x0027AE60
#define CLR_ERR         0x00E74C3C

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
#define WIN_TOP     60
#define WIN_LEFT    100
#define WIN_WIDTH   600
#define WIN_HEIGHT  400
#define WIN_RIGHT   (WIN_LEFT + WIN_WIDTH - 1)
#define WIN_BOTTOM  (WIN_TOP + WIN_HEIGHT - 1)

#define TASKBAR_H   40
#define TASKBAR_Y   (600 - TASKBAR_H)

#define MENU_WIDTH  200
#define MENU_ITEM_COUNT 6
#define MENU_H      (MENU_ITEM_COUNT * 30 + 50)
#define MENU_Y      (TASKBAR_Y - MENU_H)

/* ═══════════════════════════════════════════════════════════════════════════ */
/* State                                                                      */
/* ═══════════════════════════════════════════════════════════════════════════ */
static AppId g_app      = APP_NONE;
static bool  g_menu_open = false;
static int   g_menu_sel  = 0;
static int   g_mouse_x   = 400;
static int   g_mouse_y   = 300;
static bool  g_mouse_ldown = false;
static bool  g_exit      = false;
static bool  g_java_on   = false;

/* Java VM state */
static java_result_t g_java_result;
static bool g_java_ran   = false;
static int  g_java_demo  = 0;
static uint32_t g_tick   = 0;

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Helpers                                                                    */
/* ═══════════════════════════════════════════════════════════════════════════ */
static int slen(const char *s) { return (int)strlen(s); }

static void clamp_mouse(void) {
    if (g_mouse_x < 0) g_mouse_x = 0;
    if (g_mouse_x >= fb_width) g_mouse_x = fb_width - 1;
    if (g_mouse_y < 0) g_mouse_y = 0;
    if (g_mouse_y >= fb_height) g_mouse_y = fb_height - 1;
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
/* Desktop Wallpaper                                                          */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void draw_wallpaper(void) {
    /* Draw a simple gradient from KDE-ish dark blue to lighter blue */
    for (int y = 0; y < fb_height - TASKBAR_H; y++) {
        uint32_t r = 30 + (y * 50 / fb_height);
        uint32_t g = 60 + (y * 80 / fb_height);
        uint32_t b = 100 + (y * 120 / fb_height);
        uint32_t col = (r << 16) | (g << 8) | b;
        fb_fill_rect(0, y, fb_width, 1, col);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Desktop Icons                                                              */
/* ═══════════════════════════════════════════════════════════════════════════ */
struct DesktopIcon {
    int x, y;
    const char *label;
    AppId app;
    uint32_t color;
};

static DesktopIcon g_icons[] = {
    {  30,  30, "Files",   APP_FILES,    0x00F39C12 }, /* Orange Folderish */
    {  30, 110, "Term",    APP_TERMINAL, 0x002C3E50 }, /* Dark Console */
    {  30, 190, "Network", APP_NET,      0x003498DB }, /* Blue Net */
    {  30, 270, "Java",    APP_JAVA,     0x00E74C3C }, /* Red Cup */
};
#define ICON_COUNT 4

static void draw_icons(void) {
    for (int i = 0; i < ICON_COUNT; i++) {
        DesktopIcon &ic = g_icons[i];
        bool hot = hit(g_mouse_x, g_mouse_y, ic.x - 10, ic.y - 10, 60, 70);
        
        if (hot) {
            fb_fill_rect(ic.x - 10, ic.y - 10, 60, 70, 0x40FFFFFF); /* Hover highlight */
        }
        
        /* Draw a big 32x32 rectangular icon for now */
        fb_fill_rect(ic.x + 4, ic.y, 32, 32, ic.color);
        fb_draw_rect(ic.x + 4, ic.y, 32, 32, 0x00FFFFFF);
        
        /* Icon shadow text */
        fb_draw_string(ic.x, ic.y + 40 + 1, ic.label, 0x00000000, 0xFF000000); // Shadow
        fb_draw_string(ic.x, ic.y + 40, ic.label, CLR_TEXT_LIGHT, 0xFF000000);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Taskbar                                                                    */
/* ═══════════════════════════════════════════════════════════════════════════ */
struct TabBtn {
    const char *label;
    AppId app;
    int x, w;
};

static TabBtn g_tabs[] = {
    { "Home",  APP_HOME,  0, 0 },
    { "Files", APP_FILES, 0, 0 },
    { "Net",   APP_NET,   0, 0 },
    { "Java",  APP_JAVA,  0, 0 },
};
#define TAB_COUNT 4

static void draw_taskbar(void) {
    fb_fill_rect(0, TASKBAR_Y, fb_width, TASKBAR_H, CLR_PANEL);
    fb_fill_rect(0, TASKBAR_Y, fb_width, 1, CLR_PANEL_EDGE); /* Top Border */

    /* Start button */
    bool start_hot = hit(g_mouse_x, g_mouse_y, 0, TASKBAR_Y, 100, TASKBAR_H);
    uint32_t start_col = g_menu_open ? CLR_TAB_HOT : (start_hot ? CLR_TAB_HOT : CLR_PANEL);
    fb_fill_rect(0, TASKBAR_Y + 1, 100, TASKBAR_H - 1, start_col);
    fb_draw_string(16, TASKBAR_Y + 12, "[K] Menu", CLR_TEXT_DARK, 0xFF000000);
    fb_fill_rect(100, TASKBAR_Y + 4, 1, TASKBAR_H - 8, CLR_PANEL_EDGE);

    /* Tab buttons */
    int x = 110;
    for (int i = 0; i < TAB_COUNT; i++) {
        g_tabs[i].x = x;
        int w = slen(g_tabs[i].label) * 8 + 32;
        g_tabs[i].w = w;
        
        bool active = (g_app == g_tabs[i].app);
        bool hot = hit(g_mouse_x, g_mouse_y, x, TASKBAR_Y, w, TASKBAR_H);
        
        uint32_t bg = active ? CLR_TAB_ACT : (hot ? CLR_TAB_HOT : CLR_PANEL);
        fb_fill_rect(x, TASKBAR_Y + 1, w, TASKBAR_H - 1, bg);
        if (active) {
            fb_fill_rect(x, TASKBAR_Y + TASKBAR_H - 3, w, 3, CLR_WIN_TITLE); /* Active underline */
        }
        
        fb_draw_string(x + 16, TASKBAR_Y + 12, g_tabs[i].label, CLR_TEXT_DARK, 0xFF000000);
        x += w + 4;
    }

    /* System tray */
    int tray_x = fb_width - 150;
    fb_fill_rect(tray_x - 10, TASKBAR_Y + 4, 1, TASKBAR_H - 8, CLR_PANEL_EDGE);
    
    net_info_t info;
    network_get_info(&info);
    if (info.present && info.ready) {
        fb_draw_string(tray_x, TASKBAR_Y + 12, "NET", CLR_OK, 0xFF000000);
    } else {
        fb_draw_string(tray_x, TASKBAR_Y + 12, "NET", CLR_ERR, 0xFF000000);
    }
    
    if (g_java_on) {
        fb_draw_string(tray_x + 40, TASKBAR_Y + 12, "JAVA", CLR_WIN_TITLE, 0xFF000000);
    }

    /* Clock */
    int h, m;
    get_time(&h, &m);
    char hh[3], mm[3];
    pad2(h, hh); pad2(m, mm);
    char clock_str[6] = { hh[0], hh[1], ':', mm[0], mm[1], '\0' };
    fb_draw_string(fb_width - 50, TASKBAR_Y + 12, clock_str, CLR_TEXT_DARK, 0xFF000000);
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

    /* Drop shadow */
    fb_fill_rect(0 + 4, MENU_Y + 4, MENU_WIDTH, MENU_H, CLR_WIN_SHADOW);
    
    fb_fill_rect(0, MENU_Y, MENU_WIDTH, MENU_H, CLR_MENU);
    fb_draw_rect(0, MENU_Y, MENU_WIDTH, MENU_H, CLR_WIN_BORDER);

    /* Header */
    fb_fill_rect(0, MENU_Y, MENU_WIDTH, 40, CLR_WIN_TITLE);
    fb_draw_string(20, MENU_Y + 12, "HeatOS Plasma", CLR_TEXT_LIGHT, 0xFF000000);

    /* Items */
    int item_y = MENU_Y + 40 + 10;
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        bool sel = (i == g_menu_sel);
        uint32_t bg = sel ? CLR_MENU_SEL : CLR_MENU;
        uint32_t fg = sel ? CLR_MENU_SEL_T : CLR_TEXT_DARK;
        
        fb_fill_rect(10, item_y, MENU_WIDTH - 20, 30, bg);
        fb_draw_string(30, item_y + 7, g_menu_items[i].label, fg, 0xFF000000);
        item_y += 30;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Window Frame                                                               */
/* ═══════════════════════════════════════════════════════════════════════════ */
static const char *app_title(AppId app) {
    switch (app) {
        case APP_HOME:  return "Home";
        case APP_FILES: return "Files";
        case APP_NET:   return "Network settings";
        case APP_JAVA:  return "Java Virtual Machine";
        default:        return "Window";
    }
}

static void draw_window(void) {
    if (g_app < APP_HOME || g_app > APP_JAVA) return;

    /* Drop shadow */
    fb_fill_rect(WIN_LEFT + 8, WIN_TOP + 8, WIN_WIDTH, WIN_HEIGHT, CLR_WIN_SHADOW);

    /* Body */
    fb_fill_rect(WIN_LEFT, WIN_TOP, WIN_WIDTH, WIN_HEIGHT, CLR_WIN_BODY);
    fb_draw_rect(WIN_LEFT, WIN_TOP, WIN_WIDTH, WIN_HEIGHT, CLR_WIN_BORDER);

    /* Title bar */
    fb_fill_rect(WIN_LEFT, WIN_TOP, WIN_WIDTH, 30, CLR_WIN_TITLE);
    fb_draw_string(WIN_LEFT + 20, WIN_TOP + 7, app_title(g_app), CLR_TEXT_LIGHT, 0xFF000000);

    /* Close [X] button */
    bool close_hot = hit(g_mouse_x, g_mouse_y, WIN_RIGHT - 40, WIN_TOP, 40, 30);
    if (close_hot) {
        fb_fill_rect(WIN_RIGHT - 40, WIN_TOP, 40, 30, CLR_ERR);
    }
    fb_draw_string(WIN_RIGHT - 24, WIN_TOP + 7, "X", CLR_TEXT_LIGHT, 0xFF000000);
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* App Content Renderers                                                      */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void render_home(void) {
    int x = WIN_LEFT + 30, y = WIN_TOP + 50;

    fb_draw_string(x, y, "Welcome to full high-resolution Popeye Plasma!", CLR_TEXT_DARK, 0xFF000000); y += 30;

    fb_draw_string(x, y, "System Engine:    Bochs Graphics Adapter & LFB", CLR_TEXT_DIM, 0xFF000000); y += 20;
    fb_draw_string(x, y, "Resolution:       800x600 32-bit ARGB", CLR_TEXT_DIM, 0xFF000000); y += 40;

    fb_draw_string(x, y, "Use your mouse to interact.", CLR_TEXT_DARK, 0xFF000000); y += 20;
    fb_draw_string(x, y, "F2 or T           Exit back to text-mode terminal", CLR_TEXT_DIM, 0xFF000000); y += 20;
}

static void render_files(void) {
    int x = WIN_LEFT + 30, y = WIN_TOP + 50;
    fb_draw_string(x, y, "File System explorer placeholder", CLR_TEXT_DARK, 0xFF000000); y += 30;

    const char *dirs[] = { "/apps", "/docs", "/home", "/java", "/plasma", "/system" };
    for (int i = 0; i < 6; i++) {
        fb_draw_string(x, y, dirs[i], CLR_WIN_TITLE, 0xFF000000); y += 20;
    }
}

static void render_net(void) {
    int x = WIN_LEFT + 30, y = WIN_TOP + 50;
    net_info_t info;
    network_get_info(&info);

    if (!info.present) {
        fb_draw_string(x, y, "Adapter: Not detected", CLR_ERR, 0xFF000000);
        return;
    }

    char ip[24], gw[24], nm[24];
    format_ip(info.ip, ip);
    format_ip(info.gateway, gw);
    format_ip(info.netmask, nm);

    fb_draw_string(x, y, "Adapter:  NE2000 PCI", CLR_TEXT_DARK, 0xFF000000); y += 20;
    fb_draw_string(x, y, "State:    ", CLR_TEXT_DARK, 0xFF000000);
    fb_draw_string(x + 80, y, info.ready ? "Online" : "Booting", info.ready ? CLR_OK : CLR_ERR, 0xFF000000); y += 40;

    if (info.ready) {
        fb_draw_string(x, y, "IP:       ", CLR_TEXT_DIM, 0xFF000000); fb_draw_string(x + 80, y, ip, CLR_TEXT_DARK, 0xFF000000); y += 20;
        fb_draw_string(x, y, "Gateway:  ", CLR_TEXT_DIM, 0xFF000000); fb_draw_string(x + 80, y, gw, CLR_TEXT_DARK, 0xFF000000); y += 20;
        fb_draw_string(x, y, "Netmask:  ", CLR_TEXT_DIM, 0xFF000000); fb_draw_string(x + 80, y, nm, CLR_TEXT_DARK, 0xFF000000); y += 20;
    }
}

static void render_java(void) {
    int x = WIN_LEFT + 30, y = WIN_TOP + 50;

    fb_draw_string(x, y, "Java Support: ", CLR_TEXT_DARK, 0xFF000000);
    fb_draw_string(x + 120, y, g_java_on ? "Enabled" : "Disabled", g_java_on ? CLR_OK : CLR_TEXT_DIM, 0xFF000000); y += 30;

    int demo_count = java_vm_demo_count();
    fb_draw_string(x, y, "Demo:         ", CLR_TEXT_DARK, 0xFF000000);
    if (demo_count > 0)
        fb_draw_string(x + 120, y, java_vm_demo_name(g_java_demo), CLR_WIN_TITLE, 0xFF000000);
    y += 20;
    
    fb_draw_string(x, y, "[N] Next demo   [R/Enter] Run", CLR_TEXT_DIM, 0xFF000000); y += 40;

    /* Output area */
    if (g_java_ran) {
        if (g_java_result.success) {
            fb_draw_string(x, y, "Output:", CLR_TEXT_DARK, 0xFF000000); y += 20;
            const char *p = g_java_result.output;
            while (*p && y < WIN_BOTTOM - 20) {
                int len = 0;
                while (p[len] && p[len] != '\n' && len < 60) len++;
                char tmp[100];
                for(int i=0; i<len; i++) tmp[i] = p[i];
                tmp[len] = '\0';
                fb_draw_string(x, y, tmp, CLR_OK, 0xFF000000);
                y += 16;
                p += len;
                if (*p == '\n') p++;
            }
        } else {
            fb_draw_string(x, y, "Error: ", CLR_ERR, 0xFF000000);
            fb_draw_string(x + 60, y, g_java_result.error, CLR_ERR, 0xFF000000);
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
/* Mouse Cursor (Graphical pointer)                                           */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void draw_cursor(void) {
    /* Draw a simple 12x18 arrow pointer */
    for (int r = 0; r < 14; r++) {
        for (int c = 0; c <= r/2 + 2; c++) {
            if (c == 0 || c == r/2 + 2 || r == 13)
                fb_put_pixel(g_mouse_x + c, g_mouse_y + r, 0xFF000000); /* Black outline */
            else
                fb_put_pixel(g_mouse_x + c, g_mouse_y + r, 0xFFFFFFFF); /* White fill */
        }
    }
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
        fb_fill_rect(0, 0, fb_width, fb_height, 0x00000000);
        fb_draw_string(fb_width/2 - 60, fb_height/2, "System halted.", 0xFFFFFFFF, 0xFF000000);
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
    if (hit(mx, my, 0, TASKBAR_Y, 100, TASKBAR_H)) {
        g_menu_open = !g_menu_open;
        return;
    }

    /* Taskbar: Tab buttons */
    if (my >= TASKBAR_Y) {
        for (int i = 0; i < TAB_COUNT; i++) {
            if (hit(mx, my, g_tabs[i].x, TASKBAR_Y, g_tabs[i].w, TASKBAR_H)) {
                activate_app(g_tabs[i].app);
                return;
            }
        }
    }

    /* Start menu items */
    if (g_menu_open) {
        if (hit(mx, my, 0, MENU_Y, MENU_WIDTH, MENU_H)) {
            int item_y = MENU_Y + 50;
            for (int i = 0; i < MENU_ITEM_COUNT; i++) {
                if (hit(mx, my, 10, item_y, MENU_WIDTH - 20, 30)) {
                    g_menu_sel = i;
                    activate_app(g_menu_items[i].app);
                    return;
                }
                item_y += 30;
            }
            return;
        }
        /* Click outside menu closes it */
        g_menu_open = false;
        return;
    }

    /* Window close button [X] */
    if (g_app >= APP_HOME && g_app <= APP_JAVA) {
        if (hit(mx, my, WIN_RIGHT - 40, WIN_TOP, 40, 30)) {
            g_app = APP_NONE;
            return;
        }
    }

    /* Desktop icons */
    for (int i = 0; i < ICON_COUNT; i++) {
        if (hit(mx, my, g_icons[i].x - 10, g_icons[i].y - 10, 60, 70)) {
            activate_app(g_icons[i].app);
            return;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Input: Keyboard                                                            */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void handle_key(int key) {
    if (g_menu_open) {
        if (key == KEY_UP)   { g_menu_sel = (g_menu_sel + MENU_ITEM_COUNT - 1) % MENU_ITEM_COUNT; return; }
        if (key == KEY_DOWN) { g_menu_sel = (g_menu_sel + 1) % MENU_ITEM_COUNT; return; }
        if (key == KEY_ENTER) { activate_app(g_menu_items[g_menu_sel].app); return; }
        if (key == KEY_ESCAPE) { g_menu_open = false; return; }
    }

    if (key == KEY_TAB || key == 'm' || key == 'M') { g_menu_open = !g_menu_open; return; }
    if (key == KEY_F2 || key == 't' || key == 'T')   { g_exit = true; return; }
    if (key == KEY_ESCAPE) {
        if (g_app != APP_NONE) g_app = APP_NONE;
        else g_exit = true;
        return;
    }

    if (key == '1') { activate_app(APP_HOME);  return; }
    if (key == '2') { activate_app(APP_FILES); return; }
    if (key == '3') { activate_app(APP_NET);   return; }
    if (key == '4') { activate_app(APP_JAVA);  return; }

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
    
    fb_swap(); // Reserved for double buffering
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Main Desktop Loop                                                          */
/* ═══════════════════════════════════════════════════════════════════════════ */
bool popeye_plasma_desktop_run(bool java_enabled) {
    /* Initialize Framebuffer and High-Resolution mode */
    if (!fb_init()) {
        return false;
    }

    g_app        = APP_NONE;
    g_menu_open  = false;
    g_menu_sel   = 0;
    g_mouse_x    = fb_width / 2;
    g_mouse_y    = fb_height / 2;
    g_mouse_ldown = false;
    g_exit       = false;
    g_java_on    = java_enabled;
    g_java_ran   = false;

    // Flush any pending keyboard input
    while (keyboard_poll() != 0) {}

    while (!g_exit) {
        /* Pseudo-event tick for animation/checks */
        g_tick++;
        if ((g_tick & 0x7FF) == 0) network_poll();

        /* Input */
        int key = keyboard_poll();
        if (key != 0) handle_key(key);

        mouse_packet_t mp;
        if (mouse_poll(&mp)) {
            g_mouse_x += mp.dx;
            g_mouse_y -= mp.dy;
            clamp_mouse();

            bool ldown = mp.left;
            if (ldown && !g_mouse_ldown) handle_click();
            g_mouse_ldown = ldown;
        }

        render_frame();
    }

    /* Restore VGA text mode before exiting */
    bga_set_video_mode(0, 0, 0, false, false);
    
    return true;
}
