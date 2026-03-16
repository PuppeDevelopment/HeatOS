/*=============================================================================
 * XFCE-Inspired Desktop Environment (formerly Popeye Plasma)
 * Multi-window compositor, real graphics API, File Manager (create/delete files)
 *===========================================================================*/
extern "C" {
#include "framebuffer.h"
#include "keyboard.h"
#include "mouse.h"
#include "network.h"
#include "string.h"
#include "io.h"
#include "ramdisk.h"
#include "plasma_java.h"
#include "bga.h"
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Color Theme - XFCE Default Dark Theme                                      */
/* ═══════════════════════════════════════════════════════════════════════════ */
#define CLR_TEXT_DARK   0x00DCDCDC
#define CLR_TEXT_LIGHT  0x00FFFFFF
#define CLR_TEXT_DIM    0x00888888

#define CLR_DESKTOP     0x002B4B6E // XFCE classic blueish background
#define CLR_PANEL       0x002D2D2D // Dark grey top panel
#define CLR_PANEL_T     0x00FFFFFF

#define CLR_MENU        0x00333333
#define CLR_MENU_SEL    0x004A90E2
#define CLR_MENU_SEL_T  0x00FFFFFF

#define CLR_WIN_BODY    0x003A3A3A // Dark window body
#define CLR_WIN_TITLE   0x00282828 // Darker title
#define CLR_WIN_TITLE_U 0x00353535 // Unfocused title
#define CLR_WIN_BORDER  0x001A1A1A
#define CLR_WIN_SHADOW  0x00111111

#define CLR_BROWSER_BAR 0x002D2D2D
#define CLR_LINK        0x004A90E2

#define CLR_BTN_CLOSE_H 0x00C9302C
#define CLR_BTN_CLOSE   0x008A201E

#define CLR_ACCENT      0x004A90E2
#define CLR_OK          0x005CB85C
#define CLR_ERR         0x00D9534F
#define CLR_FILE_BG     0x00444444

/* ═══════════════════════════════════════════════════════════════════════════ */
/* State and Definitions                                                      */
/* ═══════════════════════════════════════════════════════════════════════════ */
#define PANEL_H   32

enum AppId {
    APP_NONE = 0,
    APP_HOME,     /* Settings/About */
    APP_FILES,    /* Thunar-like File Manager */
    APP_BROWSER,  /* Midori-like Browser */
    APP_NET,
    APP_COUNT
};

struct MenuEntry { AppId id; const char *label; };
static MenuEntry g_menu_items[] = {
    { APP_HOME,    "Settings" },
    { APP_FILES,   "File Manager" },
    { APP_BROWSER, "Web Browser" },
    { APP_NET,     "Network Manager" },
};
#define MENU_ITEM_COUNT (sizeof(g_menu_items)/sizeof(g_menu_items[0]))

struct Window {
    bool active;
    AppId app;
    int x, y, width, height;
    bool focused;
};

#define MAX_WINDOWS 6
static Window g_windows[MAX_WINDOWS];

static bool g_menu_open  = false;
static int  g_menu_sel   = -1;
static bool g_exit       = false;
static int  g_mouse_x    = 400;
static int  g_mouse_y    = 300;
static bool g_mouse_ldown = false;
static bool g_was_ldown  = false;

/* Window dragging states */
static Window* g_drag_win = 0;
static int g_drag_offset_x = 0;
static int g_drag_offset_y = 0;

static bool hit(int x, int y, int left, int top, int w, int h) {
    return x >= left && x < left + w && y >= top && y < top + h;
}

static const char *app_title(AppId app) {
    if (app == APP_HOME) return "Settings";
    if (app == APP_FILES) return "File Manager";
    if (app == APP_BROWSER) return "Web Browser";
    if (app == APP_NET) return "Network Status";
    return "Application";
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Window Manager Functions                                                   */
/* ═══════════════════════════════════════════════════════════════════════════ */

static void wm_open_window(AppId app) {
    for(int i=0; i<MAX_WINDOWS; i++) {
        if(g_windows[i].active && g_windows[i].app == app) {
            Window temp = g_windows[i];
            for(int j=i; j<MAX_WINDOWS-1; j++) g_windows[j] = g_windows[j+1];
            g_windows[MAX_WINDOWS-1] = temp;
            for(int k=0; k<MAX_WINDOWS; k++) g_windows[k].focused = false;
            g_windows[MAX_WINDOWS-1].focused = true;
            return;
        }
    }
    
    int free_slot = -1;
    for(int i=0; i<MAX_WINDOWS; i++) {
        if(!g_windows[i].active) { free_slot = i; break; }
    }
    
    if (free_slot != -1) {
        Window w;
        w.active = true;
        w.app = app;
        w.x = 100 + (free_slot * 30);
        w.y = 80 + (free_slot * 30);
        w.width = 540;
        w.height = 360;
        if(app == APP_BROWSER) { w.width = 640; w.height = 420; }
        if(app == APP_FILES) { w.width = 600; w.height = 400; }
        
        for(int j=free_slot; j<MAX_WINDOWS-1; j++) {
            g_windows[j] = g_windows[j+1];
        }
        for(int k=0; k<MAX_WINDOWS; k++) g_windows[k].focused = false;
        w.focused = true;
        g_windows[MAX_WINDOWS-1] = w;
    }
}

static void wm_close_window(int idx) {
    g_windows[idx].active = false;
    for(int i=idx; i>0; i--) {
        g_windows[i] = g_windows[i-1];
    }
    g_windows[0].active = false;
    for(int i=MAX_WINDOWS-1; i>=0; i--) {
        if(g_windows[i].active) {
            g_windows[i].focused = true;
            break;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Renderers                                                                  */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void draw_wallpaper() {
    fb_fill_rect(0, 0, fb_width, fb_height, CLR_DESKTOP);
    fb_draw_string(fb_width - 150, fb_height - 30, "XFCE HeatOS", 0x00FFFFFF, 0xFF000000);
}

static void draw_panel() {
    fb_fill_rect(0, 0, fb_width, PANEL_H, CLR_PANEL);
    fb_draw_line(0, PANEL_H, fb_width, PANEL_H, CLR_WIN_BORDER);
    
    // Whisker Menu Button
    fb_fill_rect(5, 4, 100, 24, g_menu_open ? CLR_ACCENT : 0x00444444);
    fb_draw_string(15, 8, "Applications", CLR_PANEL_T, 0xFF000000);
    
    // Open apps
    int x = 115;
    for(int i=MAX_WINDOWS-1; i>=0; i--) {
        if(g_windows[i].active) {
            fb_fill_rect(x, 4, 120, 24, g_windows[i].focused ? 0x00555555 : 0x00333333);
            fb_draw_string(x+8, 8, app_title(g_windows[i].app), CLR_PANEL_T, 0xFF000000);
            x += 125;
        }
    }
    
    // Clock
    fb_draw_string(fb_width - 70, 8, "12:00 PM", CLR_PANEL_T, 0xFF000000);
}

static void draw_menu() {
    if (!g_menu_open) return;
    int tw = 200;
    int th = MENU_ITEM_COUNT * 30 + 10;
    
    // Shadow
    fb_fill_rect(5, PANEL_H + 5, tw, th, CLR_WIN_SHADOW);
    fb_fill_rect(0, PANEL_H, tw, th, CLR_MENU);
    fb_draw_rect(0, PANEL_H, tw, th, CLR_WIN_BORDER);
    
    int my = PANEL_H + 5;
    g_menu_sel = -1;
    for (int i=0; i<(int)MENU_ITEM_COUNT; i++) {
        if(hit(g_mouse_x, g_mouse_y, 0, my, tw, 30)) g_menu_sel = i;
        uint32_t bg = (g_menu_sel == i) ? CLR_MENU_SEL : CLR_MENU;
        uint32_t fg = CLR_TEXT_LIGHT;
        
        fb_fill_rect(0, my, tw, 30, bg);
        fb_draw_string(15, my + 8, g_menu_items[i].label, fg, 0xFF000000);
        my += 30;
    }
}

static void draw_files(Window& w) {
    int cx = w.x + 2; int cy = w.y + 32;
    int cw = w.width - 4; int ch = w.height - 34;

    // Sidebar
    fb_fill_rect(cx, cy, 140, ch, CLR_WIN_TITLE_U);
    fb_draw_line(cx+140, cy, cx+140, cy+ch, CLR_WIN_BORDER);
    fb_draw_string(cx+10, cy+15, "[ Home ]", CLR_TEXT_LIGHT, 0xFF000000);
    fb_draw_string(cx+10, cy+45, "[ Desktop ]", CLR_TEXT_DARK, 0xFF000000);
    
    // Toolbar (Make File, Delete File)
    fb_fill_rect(cx+141, cy, cw-141, 40, CLR_MENU);
    fb_draw_line(cx+141, cy+40, cx+cw, cy+40, CLR_WIN_BORDER);
    
    // Buttons (simulating interactions)
    fb_fill_rect(cx+150, cy+8, 100, 24, CLR_ACCENT);
    fb_draw_string(cx+160, cy+12, "+ New File", CLR_TEXT_LIGHT, 0xFF000000);
    
    // File view
    int fx = cx + 150;
    int fy = cy + 50;
    
    fs_node_t root = 0; // ROOT node is technically 0 for ramdisk listing setup
    fs_node_t iter;
    int idx = 0;

    if (fs_list_begin(root, &iter)) {
        fs_node_t child;
        while (fs_list_next(&iter, &child)) {
            bool isdir = fs_is_dir(child);
            const char *name = fs_get_name(child);
            
            // Draw tile
            bool hover = hit(g_mouse_x, g_mouse_y, fx, fy, 80, 80);
            fb_fill_rect(fx, fy, 80, 80, hover ? CLR_ACCENT : CLR_FILE_BG);
            fb_draw_rect(fx, fy, 80, 80, CLR_WIN_BORDER);
            
            // Delete button on top right of file tile
            bool del_hover = hit(g_mouse_x, g_mouse_y, fx+65, fy+5, 12, 12);
            fb_fill_rect(fx+65, fy+5, 12, 12, del_hover ? CLR_ERR : CLR_BTN_CLOSE);
            fb_draw_string(fx+68, fy+5, "x", CLR_TEXT_LIGHT, 0xFF000000);
            if (del_hover && g_mouse_ldown && !g_was_ldown) {
                fs_delete(child);
                return; // Break out of draw this frame since fs changed
            }
            
            fb_draw_string(fx+10, fy+40, isdir ? "DIR" : "FILE", CLR_TEXT_LIGHT, 0xFF000000);
            fb_draw_string(fx+10, fy+60, name, CLR_TEXT_LIGHT, 0xFF000000);
            
            fx += 90;
            idx++;
            if (idx > 4) { idx=0; fx = cx + 150; fy += 90; }
        }
    }
    
    // Handle New File Click
    if (hit(g_mouse_x, g_mouse_y, cx+150, cy+8, 100, 24) && g_mouse_ldown && !g_was_ldown) {
        static int fnum = 0;
        char buf[12];
        strcpy(buf, "test");
        utoa((uint32_t)fnum++, buf+4, 10);
        fs_create_file(0, buf);
    }
}

static void draw_home(Window& w) {
    int cx = w.x + 20; int cy = w.y + 50;
    fb_draw_string(cx, cy, "XFCE Settings App", CLR_TEXT_LIGHT, 0xFF000000); cy += 30;
    fb_draw_string(cx, cy, "OS:          HeatOS 32-bit", CLR_TEXT_DIM, 0xFF000000); cy += 20;
    fb_draw_string(cx, cy, "Resolution:  800x600 Double Buffered", CLR_TEXT_DIM, 0xFF000000); cy += 20;
    fb_draw_string(cx, cy, "RAMFS:       Supports modify/delete", CLR_TEXT_DIM, 0xFF000000); cy += 30;
    fb_draw_string(cx, cy, "Try out the File Manager!", CLR_ACCENT, 0xFF000000);
}

static void draw_windows() {
    for (int i=0; i<MAX_WINDOWS; i++) {
        if(!g_windows[i].active) continue;
        Window& w = g_windows[i];
        
        // Shadow
        fb_fill_rect(w.x + 8, w.y + 8, w.width, w.height, CLR_WIN_SHADOW);
        
        // Body
        fb_fill_rect(w.x, w.y, w.width, w.height, CLR_WIN_BODY);
        fb_draw_rect(w.x, w.y, w.width, w.height, CLR_WIN_BORDER);
        
        // Title bar
        uint32_t t_col = w.focused ? CLR_WIN_TITLE : CLR_WIN_TITLE_U;
        fb_fill_rect(w.x, w.y, w.width, 30, t_col);
        fb_draw_string(w.x + 10, w.y + 8, app_title(w.app), CLR_TEXT_LIGHT, 0xFF000000);
        
        // Close Button
        bool hover_close = hit(g_mouse_x, g_mouse_y, w.x + w.width - 40, w.y, 40, 30);
        fb_fill_rect(w.x + w.width - 40, w.y, 40, 30, hover_close ? CLR_BTN_CLOSE_H : CLR_WIN_TITLE_U);
        fb_draw_string(w.x + w.width - 25, w.y + 8, "X", CLR_TEXT_LIGHT, 0xFF000000);
        
        if(w.app == APP_HOME) draw_home(w);
        else if(w.app == APP_FILES) draw_files(w);
        // Add stubs for others... 
    }
}

static void draw_cursor() {
    int mx = g_mouse_x, my = g_mouse_y;
    // Outer white border
    fb_draw_line(mx, my, mx+16, my+10, CLR_TEXT_LIGHT);
    fb_draw_line(mx, my, mx+10, my+16, CLR_TEXT_LIGHT);
    fb_draw_line(mx+16, my+10, mx+10, my+16, CLR_TEXT_LIGHT);
    // Inner black fill
    for(int y=0; y<16; y++) {
        for(int x=0; x<16; x++) {
            if (x+y < 16 && x > 0 && y > 0 && y < x+10 && x < y+10) {
                fb_put_pixel(mx+x, my+y, 0);
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Interactions                                                               */
/* ═══════════════════════════════════════════════════════════════════════════ */

static void handle_clicks() {
    bool clicked = (g_mouse_ldown && !g_was_ldown);
    bool dragging = g_mouse_ldown;
    
    if (dragging && g_drag_win) {
        g_drag_win->x = g_mouse_x - g_drag_offset_x;
        g_drag_win->y = g_mouse_y - g_drag_offset_y;
        return;
    } else { g_drag_win = 0; }

    if (!clicked) return;

    if (hit(g_mouse_x, g_mouse_y, 5, 4, 100, 24)) {
        g_menu_open = !g_menu_open;
        return;
    }
    
    if (g_menu_open) {
        int tw = 200;
        int th = MENU_ITEM_COUNT * 30 + 10;
        if (hit(g_mouse_x, g_mouse_y, 0, PANEL_H, tw, th)) {
            if (g_menu_sel >= 0) wm_open_window(g_menu_items[g_menu_sel].id);
            g_menu_open = false;
            return;
        } else {
            g_menu_open = false; // clicked outside menu
        }
    }
    
    for (int i=MAX_WINDOWS-1; i>=0; i--) {
        if (!g_windows[i].active) continue;
        Window& w = g_windows[i];
        
        if (hit(g_mouse_x, g_mouse_y, w.x, w.y, w.width, w.height)) {
            Window temp = w;
            for(int j=i; j<MAX_WINDOWS-1; j++) g_windows[j] = g_windows[j+1];
            g_windows[MAX_WINDOWS-1] = temp;
            
            for(int k=0; k<MAX_WINDOWS; k++) g_windows[k].focused = false;
            g_windows[MAX_WINDOWS-1].focused = true;
            
            Window& twr = g_windows[MAX_WINDOWS-1];
            
            if (hit(g_mouse_x, g_mouse_y, twr.x + twr.width - 40, twr.y, 40, 30)) {
                wm_close_window(MAX_WINDOWS-1);
                return;
            }
            
            if (hit(g_mouse_x, g_mouse_y, twr.x, twr.y, twr.width - 40, 30)) {
                g_drag_win = &g_windows[MAX_WINDOWS-1];
                g_drag_offset_x = g_mouse_x - twr.x;
                g_drag_offset_y = g_mouse_y - twr.y;
            }
            return;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Main Loop                                                                  */
/* ═══════════════════════════════════════════════════════════════════════════ */
bool popeye_plasma_desktop_run(bool java_enabled) {
    (void)java_enabled;
    if (!fb_init()) return false;

    mouse_set_reporting(true);

    g_exit = false;
    g_drag_win = 0;
    g_mouse_ldown = false;
    g_was_ldown = false;
    
    for(int i=0; i<MAX_WINDOWS; i++) {
        g_windows[i].active = false;
        g_windows[i].focused = false;
    }
    
    wm_open_window(APP_HOME);
    wm_open_window(APP_FILES);

    while (keyboard_poll() != 0) {}
    while (mouse_poll(0)) {}

    while (!g_exit) {
        // Drain keyboard queue
        for(int i=0; i<100; i++) {
            int key = keyboard_poll();
            if (key == 0) break;
            if (key == KEY_F2 || key == 't' || key == 'T') g_exit = true; // F2 or 't'
        }

        // Drain mouse queue
        for(int i=0; i<100; i++) {
            mouse_packet_t mp;
            if (!mouse_poll(&mp)) break;
            g_mouse_x += mp.dx;
            g_mouse_y -= mp.dy; 
            if (g_mouse_x < 0) g_mouse_x = 0;
            if (g_mouse_x > fb_width - 1) g_mouse_x = fb_width - 1;
            if (g_mouse_y < 0) g_mouse_y = 0;
            if (g_mouse_y > fb_height - 1) g_mouse_y = fb_height - 1;
            g_mouse_ldown = mp.left;
        }

        handle_clicks();
        g_was_ldown = g_mouse_ldown;

        draw_wallpaper();
        draw_windows();
        draw_panel();
        draw_menu();
        draw_cursor();
        
        fb_swap();
    }

    mouse_set_reporting(false);
    fb_shutdown();
    return true;
}

