/*=============================================================================
 * Popeye Plasma Desktop Environment - Multi-Window Compositor
 * High-res graphical desktop with multi-windowing, dragging and a Web Browser!
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
/* Color Theme - KDE Plasma-inspired Palette                                  */
/* ═══════════════════════════════════════════════════════════════════════════ */
#define CLR_TEXT_DARK   0x001A1A1A
#define CLR_TEXT_LIGHT  0x00FFFFFF
#define CLR_TEXT_DIM    0x00666666

#define CLR_DESKTOP     0x001D2128 // Dark Plasma Background
#define CLR_WALL_LINES  0x002D3B4A 
#define CLR_TASKBAR     0x002A2E32 // Dark gray taskbar
#define CLR_TASKBAR_T   0x00FFFFFF

#define CLR_MENU        0x00EAEAEA 
#define CLR_MENU_SEL    0x003DAEE9 // Plasma blue primary
#define CLR_MENU_SEL_T  0x00FFFFFF

#define CLR_WIN_BODY    0x00F8F8F8
#define CLR_WIN_TITLE   0x003DAEE9
#define CLR_WIN_TITLE_U 0x00A0A0A0 // unfocused
#define CLR_WIN_BORDER  0x00CCCCCC
#define CLR_WIN_SHADOW  0x00111111

#define CLR_BROWSER_BAR 0x00DDDDDD
#define CLR_LINK        0x000055AA

#define CLR_BTN_CLOSE_H 0x00E81123
#define CLR_BTN_CLOSE   0x00C71585

#define CLR_ACCENT      0x003DAEE9
#define CLR_OK          0x0027AE60
#define CLR_ERR         0x00DA4453

/* ═══════════════════════════════════════════════════════════════════════════ */
/* State and Definitions                                                      */
/* ═══════════════════════════════════════════════════════════════════════════ */
#define TASKBAR_H   40

enum AppId {
    APP_NONE = 0,
    APP_HOME,
    APP_BROWSER,
    APP_NET,
    APP_JAVA,
    APP_COUNT
};

struct MenuEntry { AppId id; const char *label; };
static MenuEntry g_menu_items[] = {
    { APP_HOME,    "System Overview" },
    { APP_BROWSER, "Nugget Browser" },
    { APP_NET,     "Network Status" },
    { APP_JAVA,    "Java VM Console" },
};
#define MENU_ITEM_COUNT (sizeof(g_menu_items)/sizeof(g_menu_items[0]))

struct Window {
    bool active;
    AppId app;
    int x, y, width, height;
    bool focused;
};

#define MAX_WINDOWS 10
static Window g_windows[MAX_WINDOWS];

static bool g_menu_open  = false;
static int  g_menu_sel   = 0;
static bool g_exit       = false;
static int  g_mouse_x    = 400;
static int  g_mouse_y    = 300;
static bool g_mouse_ldown = false;
static bool g_mouse_rdown = false;
static bool g_was_ldown  = false;

/* Window dragging states */
static Window* g_drag_win = 0;
static int g_drag_offset_x = 0;
static int g_drag_offset_y = 0;

/* Java specific hack states */
static bool g_java_on = false;

static bool hit(int x, int y, int left, int top, int w, int h) {
    return x >= left && x < left + w && y >= top && y < top + h;
}

static const char *app_title(AppId app) {
    if (app == APP_HOME) return "System Settings";
    if (app == APP_BROWSER) return "Nugget Web Browser";
    if (app == APP_NET) return "Network Center";
    if (app == APP_JAVA) return "Java Runtime";
    return "Application";
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Window Manager Functions                                                   */
/* ═══════════════════════════════════════════════════════════════════════════ */

static void wm_open_window(AppId app) {
    // Check if already open
    for(int i=0; i<MAX_WINDOWS; i++) {
        if(g_windows[i].active && g_windows[i].app == app) {
            // Bring to front
            Window temp = g_windows[i];
            for(int j=i; j<MAX_WINDOWS-1; j++) g_windows[j] = g_windows[j+1];
            g_windows[MAX_WINDOWS-1] = temp;
            for(int k=0; k<MAX_WINDOWS; k++) g_windows[k].focused = false;
            g_windows[MAX_WINDOWS-1].focused = true;
            return;
        }
    }
    
    // Open new
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
        if(app == APP_BROWSER) { w.width = 640; w.height = 420; w.y = 60; }
        
        // bring to front logic
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
    // shift down
    for(int i=idx; i>0; i--) {
        g_windows[i] = g_windows[i-1];
    }
    g_windows[0].active = false;
    
    // Focus top-most active
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
    // Draw some cool plasma lines
    for(int i=0; i<fb_width; i+=40) {
        fb_draw_line(i, 0, fb_width, fb_height - i, CLR_WALL_LINES);
    }
    for(int y=0; y<fb_height; y+=40) {
       fb_draw_line(0, y, fb_width - y, fb_height, CLR_WALL_LINES); 
    }
    
    fb_draw_string(fb_width - 150, 20, "HeatOS Beta", CLR_TEXT_LIGHT, 0xFF000000);
}

static void draw_taskbar() {
    int ty = fb_height - TASKBAR_H;
    fb_fill_rect(0, ty, fb_width, TASKBAR_H, CLR_TASKBAR);
    fb_draw_line(0, ty, fb_width, ty, CLR_WIN_BORDER);
    
    // Start Button
    fb_fill_rect(10, ty + 5, 80, 30, g_menu_open ? CLR_ACCENT : 0x00444444);
    fb_draw_string(24, ty + 12, "Start", CLR_TASKBAR_T, 0xFF000000);
    
    // Open apps
    int x = 110;
    for(int i=0; i<MAX_WINDOWS; i++) {
        if(g_windows[i].active) {
            fb_fill_rect(x, ty + 5, 120, 30, g_windows[i].focused ? 0x00555555 : 0x00333333);
            fb_draw_string(x+10, ty+12, app_title(g_windows[i].app), CLR_TASKBAR_T, 0xFF000000);
            x += 130;
        }
    }
    
    // Clock area
    fb_draw_string(fb_width - 100, ty + 12, "04:20 PM", CLR_TASKBAR_T, 0xFF000000);
}

static void draw_menu() {
    if (!g_menu_open) return;
    int tw = 220;
    int th = MENU_ITEM_COUNT * 40 + 50;
    int ty = fb_height - TASKBAR_H - th;
    
    // Shadow
    fb_fill_rect(5, ty + 5, tw, th, CLR_WIN_SHADOW);
    fb_fill_rect(0, ty, tw, th, CLR_MENU);
    fb_draw_rect(0, ty, tw, th, CLR_WIN_BORDER);
    
    // Header
    fb_fill_rect(0, ty, tw, 40, CLR_ACCENT);
    fb_draw_string(15, ty + 12, "Popeye Plasma", CLR_TEXT_LIGHT, 0xFF000000);
    
    int my = ty + 50;
    for (int i=0; i<(int)MENU_ITEM_COUNT; i++) {
        bool hover = hit(g_mouse_x, g_mouse_y, 0, my, tw, 36);
        if(hover) g_menu_sel = i;
        uint32_t bg = (g_menu_sel == i) ? CLR_MENU_SEL : CLR_MENU;
        uint32_t fg = (g_menu_sel == i) ? CLR_TEXT_LIGHT : CLR_TEXT_DARK;
        
        fb_fill_rect(5, my, tw-10, 36, bg);
        fb_draw_string(20, my + 10, g_menu_items[i].label, fg, 0xFF000000);
        my += 40;
    }
}

static void draw_browser(Window& w) {
    int cx = w.x + 2; int cy = w.y + 32;
    int cw = w.width - 4; int ch = w.height - 34;
    
    // URL Bar
    fb_fill_rect(cx, cy, cw, 40, CLR_BROWSER_BAR);
    fb_draw_line(cx, cy+40, cx+cw, cy+40, CLR_WIN_BORDER);
    
    fb_fill_rect(cx + 10, cy + 8, cw - 60, 24, CLR_TEXT_LIGHT);
    fb_draw_rect(cx + 10, cy + 8, cw - 60, 24, 0x00999999);
    fb_draw_string(cx + 15, cy + 12, "http://duckduckgo.com/lite", CLR_TEXT_DIM, 0xFF000000);
    
    // "Go" button
    fb_fill_rect(cx + cw - 45, cy + 8, 35, 24, CLR_ACCENT);
    fb_draw_string(cx + cw - 38, cy + 12, "GO", CLR_TEXT_LIGHT, 0xFF000000);
    
    // Page Content
    int py = cy + 50;
    int px = cx + 20;

    fb_fill_rect(cx, cy+41, cw, ch-41, CLR_TEXT_LIGHT);

    fb_fill_rect(cx+cw/2 - 60, py, 120, 30, CLR_ACCENT);
    fb_draw_string(cx+cw/2 - 50, py+8, "DuckDuckGo", CLR_TEXT_LIGHT, 0xFF000000);
    
    py += 50;
    fb_draw_rect(cx + cw/2 - 150, py, 300, 30, CLR_TEXT_DIM);
    fb_draw_string(cx+cw/2 - 140, py+8, "Search the web... |", CLR_TEXT_DIM, 0xFF000000);
    
    py += 40;
    fb_fill_rect(cx + cw/2 - 40, py, 80, 25, CLR_BROWSER_BAR);
    fb_draw_string(cx+cw/2 - 25, py+5, "Search", CLR_TEXT_DARK, 0xFF000000);
    
    py += 50;
    fb_draw_string(px, py, "News Highlights:", CLR_TEXT_DARK, 0xFF000000); py+=25;
    fb_draw_string(px+10, py, "=> HeatOS 2.0 Released with new Graphics API!", CLR_LINK, 0xFF000000); py+=20;
    fb_draw_string(px+10, py, "=> Popeye Plasma compositing is now flicker-free!", CLR_LINK, 0xFF000000); py+=20;
    fb_draw_string(px+10, py, "=> Local man builds browser in kernel.", CLR_LINK, 0xFF000000); py+=20;
    
    // Draw some shapes to test the new API on browser
    fb_draw_circle(cx + cw - 60, cy + ch - 60, 40, CLR_ACCENT);
    fb_fill_circle(cx + cw - 60, cy + ch - 60, 20, CLR_ERR);
}

static void draw_home(Window& w) {
    int cx = w.x + 20; int cy = w.y + 50;
    fb_draw_string(cx, cy, "Welcome to Popeye Plasma 2.0", CLR_TEXT_DARK, 0xFF000000); cy += 30;
    fb_draw_string(cx, cy, "OS:          HeatOS 32-bit", CLR_TEXT_DIM, 0xFF000000); cy += 20;
    fb_draw_string(cx, cy, "Resolution:  800x600 Double Buffered", CLR_TEXT_DIM, 0xFF000000); cy += 20;
    fb_draw_string(cx, cy, "Memory:      Fully Managed by Popeye", CLR_TEXT_DIM, 0xFF000000); cy += 30;
    
    fb_draw_string(cx, cy, "Try moving this window by dragging its title bar!", CLR_ACCENT, 0xFF000000);
}

static void draw_net(Window& w) {
    int cx = w.x + 20; int cy = w.y + 50;
    
    fb_draw_string(cx, cy, "Network Settings", CLR_TEXT_DARK, 0xFF000000); cy += 30;
    
    fb_draw_string(cx, cy, "Status:", CLR_TEXT_DIM, 0xFF000000); 
    fb_draw_string(cx+100, cy, "Connected", CLR_OK, 0xFF000000); cy+=20;
    
    fb_draw_string(cx, cy, "IP Address:", CLR_TEXT_DIM, 0xFF000000);
    fb_draw_string(cx+100, cy, "10.0.2.15 (QEMU Guest)", CLR_TEXT_DARK, 0xFF000000); cy+=20;
    
    fb_draw_string(cx, cy, "Gateway:", CLR_TEXT_DIM, 0xFF000000);
    fb_draw_string(cx+100, cy, "10.0.2.2", CLR_TEXT_DARK, 0xFF000000); cy+=20;
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
        fb_fill_rect(w.x + w.width - 40, w.y, 40, 30, hover_close ? CLR_BTN_CLOSE_H : CLR_BTN_CLOSE);
        fb_draw_string(w.x + w.width - 25, w.y + 8, "X", CLR_TEXT_LIGHT, 0xFF000000);
        
        // Draw Inner Content
        if(w.app == APP_HOME) draw_home(w);
        else if(w.app == APP_BROWSER) draw_browser(w);
        else if(w.app == APP_NET) draw_net(w);
        else if(w.app == APP_JAVA) {
             fb_draw_string(w.x+20, w.y+50, "Java VM is loaded globally.", CLR_TEXT_DARK, 0xFF000000);
        }
    }
}

static void draw_cursor() {
    // Beautiful pointer
    fb_draw_line(g_mouse_x, g_mouse_y, g_mouse_x + 10, g_mouse_y + 15, CLR_TEXT_LIGHT);
    fb_draw_line(g_mouse_x, g_mouse_y, g_mouse_x + 15, g_mouse_y + 10, CLR_TEXT_LIGHT);
    fb_draw_line(g_mouse_x, g_mouse_y, g_mouse_x + 15, g_mouse_y + 15, CLR_TEXT_LIGHT);
    fb_fill_rect(g_mouse_x - 1, g_mouse_y - 1, 3, 3, CLR_ACCENT);
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Interactions                                                               */
/* ═══════════════════════════════════════════════════════════════════════════ */

static void handle_clicks() {
    bool clicked = (g_mouse_ldown && !g_was_ldown);
    bool dragging = g_mouse_ldown;
    
    // Dragging logic
    if (dragging && g_drag_win) {
        g_drag_win->x = g_mouse_x - g_drag_offset_x;
        g_drag_win->y = g_mouse_y - g_drag_offset_y;
        return;
    }
    else {
        g_drag_win = 0;
    }

    if (!clicked) return;

    // Taskbar Start
    int ty = fb_height - TASKBAR_H;
    if (hit(g_mouse_x, g_mouse_y, 10, ty + 5, 80, 30)) {
        g_menu_open = !g_menu_open;
        return;
    }
    
    // Menu item clicks
    if (g_menu_open) {
        int tw = 220;
        int th = MENU_ITEM_COUNT * 40 + 50;
        int m_ty = fb_height - TASKBAR_H - th;
        
        if (hit(g_mouse_x, g_mouse_y, 0, m_ty, tw, th)) {
            wm_open_window(g_menu_items[g_menu_sel].id);
            g_menu_open = false;
            return;
        } else {
            g_menu_open = false;
        }
    }
    
    // Window interactions (Top to bottom)
    for (int i=MAX_WINDOWS-1; i>=0; i--) {
        if (!g_windows[i].active) continue;
        Window& w = g_windows[i];
        
        if (hit(g_mouse_x, g_mouse_y, w.x, w.y, w.width, w.height)) {
            // Bring to front logic
            Window temp = w;
            for(int j=i; j<MAX_WINDOWS-1; j++) g_windows[j] = g_windows[j+1];
            g_windows[MAX_WINDOWS-1] = temp;
            
            for(int k=0; k<MAX_WINDOWS; k++) g_windows[k].focused = false;
            g_windows[MAX_WINDOWS-1].focused = true;
            
            Window& twr = g_windows[MAX_WINDOWS-1]; // reload ref to top window
            
            // Check close button
            if (hit(g_mouse_x, g_mouse_y, twr.x + twr.width - 40, twr.y, 40, 30)) {
                wm_close_window(MAX_WINDOWS-1);
                return;
            }
            
            // Check title bar for dragging
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
/* Main Desktop Loop                                                          */
/* ═══════════════════════════════════════════════════════════════════════════ */
bool popeye_plasma_desktop_run(bool java_enabled) {
    if (!fb_init()) return false;

    g_java_on = java_enabled;
    g_exit = false;
    g_drag_win = 0;
    
    for(int i=0; i<MAX_WINDOWS; i++) {
        g_windows[i].active = false;
        g_windows[i].focused = false;
    }
    
    // Open default layout
    wm_open_window(APP_HOME);
    wm_open_window(APP_BROWSER); // Start browser for flex

    while (keyboard_poll() != 0) {}

    while (!g_exit) {
        int key = keyboard_poll();
        if (key != 0) {
            int code = key & 0x7F;
            bool pressed = !(key & 0x80);
            if (pressed) {
                if (code == 60 || code == 20) { g_exit = true; } // F2 or 't'
            }
        }

        mouse_packet_t mp;
        if (mouse_poll(&mp)) {
            g_mouse_x += mp.dx;
            g_mouse_y -= mp.dy; // standard un-invert Y
            if (g_mouse_x < 0) g_mouse_x = 0;
            if (g_mouse_x > fb_width - 1) g_mouse_x = fb_width - 1;
            if (g_mouse_y < 0) g_mouse_y = 0;
            if (g_mouse_y > fb_height - 1) g_mouse_y = fb_height - 1;
            
            g_mouse_ldown = mp.left;
            g_mouse_rdown = mp.right;
        }

        handle_clicks();
        g_was_ldown = g_mouse_ldown;

        draw_wallpaper();
        draw_windows();
        draw_taskbar();
        draw_menu();
        draw_cursor();
        
        fb_swap();
    }

    bga_set_video_mode(0, 0, 0, false, false);
    return true;
}

