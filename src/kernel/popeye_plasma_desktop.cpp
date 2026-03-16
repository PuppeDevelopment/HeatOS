/*=============================================================================
 * XFCE-inspired Popeye Plasma desktop
 * Top panel, dock, file manager, image viewer, and mini Java console.
 *===========================================================================*/
extern "C" {
#include "framebuffer.h"
#include "keyboard.h"
#include "mouse.h"
#include "string.h"
#include "ramdisk.h"
#include "plasma_java.h"
#include "image.h"
}

#define PANEL_H 32
#define DOCK_W  270
#define DOCK_H  46

#define CLR_TEXT        0x00F1F5F9
#define CLR_TEXT_DIM    0x00C8D2DC
#define CLR_TEXT_FAINT  0x00869AA9
#define CLR_PANEL_TOP   0x00373737
#define CLR_PANEL_BOT   0x00272727
#define CLR_PANEL_EDGE  0x00111111
#define CLR_MENU_BG     0x002E2E2E
#define CLR_MENU_SIDE   0x00272727
#define CLR_MENU_SEL    0x005198D2
#define CLR_MENU_ALT    0x00383838
#define CLR_WINDOW_BG   0x003A3A3A
#define CLR_WINDOW_IN   0x003F4347
#define CLR_WINDOW_TOP  0x00333437
#define CLR_WINDOW_TOPF 0x00282A2D
#define CLR_WINDOW_EDGE 0x0014171A
#define CLR_SHADOW      0x00101010
#define CLR_ACCENT      0x0055A7D9
#define CLR_FOLDER      0x00D4A64A
#define CLR_IMAGE       0x0067B6DF
#define CLR_JAVA        0x0077C46F
#define CLR_ERROR       0x00C94A4A
#define CLR_WARN        0x00E2B354
#define CLR_GOOD        0x0066BE73

enum AppId {
    APP_NONE = 0,
    APP_HOME,
    APP_FILES,
    APP_BROWSER,
    APP_NET,
    APP_JAVA,
    APP_IMAGE,
    APP_COUNT
};

struct MenuEntry {
    AppId app;
    const char *label;
    const char *hint;
    uint32_t accent;
};

struct Window {
    bool active;
    AppId app;
    fs_node_t node;
    int x;
    int y;
    int width;
    int height;
    bool focused;
    char title[24];
};

static const MenuEntry g_menu_items[] = {
    { APP_FILES,   "File Manager",  "Browse /home and /java",     CLR_FOLDER },
    { APP_BROWSER, "Web Browser",   "Midori-style mock browser",  CLR_IMAGE  },
    { APP_JAVA,    "Java Console",  "Run HeatOS mini .jar files", CLR_JAVA   },
    { APP_NET,     "Network",       "System and display status",  CLR_WARN   },
    { APP_HOME,    "Settings",      "RushOS desktop information", CLR_ACCENT },
};

#define MENU_ITEM_COUNT ((int)(sizeof(g_menu_items) / sizeof(g_menu_items[0])))
#define MAX_WINDOWS 8

static Window g_windows[MAX_WINDOWS];
static int g_window_count = 0;

static bool g_menu_open = false;
static int  g_menu_hover = -1;
static bool g_exit = false;
static int  g_mouse_x = 140;
static int  g_mouse_y = 120;
static bool g_mouse_ldown = false;
static bool g_was_ldown = false;

static Window *g_drag_win = 0;
static int g_drag_offset_x = 0;
static int g_drag_offset_y = 0;

static fs_node_t g_files_dir = 0;
static fs_node_t g_selected_node = 0;
static fs_node_t g_image_node = 0;
static image_info_t g_image_info;
static char g_image_note[64];
static fs_node_t g_java_node = 0;
static java_result_t g_java_result;
static bool g_java_ready = false;
static char g_status_line[96];

static int g_new_text_id = 0;
static int g_new_image_id = 0;
static int g_new_jar_id = 0;

static bool hit(int x, int y, int left, int top, int w, int h) {
    return x >= left && x < left + w && y >= top && y < top + h;
}

static char lower_char(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c - 'A' + 'a');
    return c;
}

static bool str_ends_with_ci(const char *text, const char *suffix) {
    size_t text_len = strlen(text);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > text_len) return false;

    text += text_len - suffix_len;
    for (size_t i = 0; i < suffix_len; i++) {
        if (lower_char(text[i]) != lower_char(suffix[i])) {
            return false;
        }
    }
    return true;
}

static bool is_image_name(const char *name) {
    return str_ends_with_ci(name, ".ppm") || str_ends_with_ci(name, ".bmp");
}

static bool is_java_name(const char *name) {
    return str_ends_with_ci(name, ".jar") ||
           str_ends_with_ci(name, ".jvm") ||
           str_ends_with_ci(name, ".jbc");
}

static uint32_t blend_color(uint32_t a, uint32_t b, int num, int den) {
    uint32_t ar = (a >> 16) & 0xFFu;
    uint32_t ag = (a >> 8) & 0xFFu;
    uint32_t ab = a & 0xFFu;
    uint32_t br = (b >> 16) & 0xFFu;
    uint32_t bg = (b >> 8) & 0xFFu;
    uint32_t bb = b & 0xFFu;
    uint32_t rr = (ar * (uint32_t)(den - num) + br * (uint32_t)num) / (uint32_t)den;
    uint32_t rg = (ag * (uint32_t)(den - num) + bg * (uint32_t)num) / (uint32_t)den;
    uint32_t rb = (ab * (uint32_t)(den - num) + bb * (uint32_t)num) / (uint32_t)den;
    return (rr << 16) | (rg << 8) | rb;
}

static void set_status(const char *text) {
    strncpy(g_status_line, text, sizeof(g_status_line) - 1);
    g_status_line[sizeof(g_status_line) - 1] = '\0';
}

static void draw_gradient_rect(int x, int y, int w, int h, uint32_t top, uint32_t bottom) {
    int den = h > 0 ? h : 1;
    for (int iy = 0; iy < h; iy++) {
        fb_fill_rect(x, y + iy, w, 1, blend_color(top, bottom, iy, den));
    }
}

static void draw_text_block(int x, int y, int w, const char *text, uint32_t color) {
    int col = 0;
    int row = 0;
    int max_cols = w / 8;

    if (max_cols < 1) max_cols = 1;

    while (*text) {
        char ch = *text++;
        if (ch == '\n') {
            col = 0;
            row++;
            continue;
        }

        if (col >= max_cols) {
            col = 0;
            row++;
        }

        fb_draw_char(x + col * 8, y + row * 16, ch, color, 0xFF000000);
        col++;
    }
}

static void build_node_path(fs_node_t node, char *buf, int buf_size) {
    if (!buf || buf_size <= 0) return;
    fs_build_path(node, buf, (size_t)buf_size);
}

static fs_node_t resolve_child(fs_node_t dir, const char *name) {
    char path[64];
    build_node_path(dir, path, sizeof(path));
    if (strcmp(path, "/") != 0) strcat(path, "/");
    strcat(path, name);
    return fs_resolve(path);
}

static fs_node_t create_file_in_dir(fs_node_t dir, const char *name, const void *data, int size) {
    fs_node_t node;
    if (!fs_create_file(dir, name)) {
        return 0;
    }

    node = resolve_child(dir, name);
    if (node != 0 && data && size > 0) {
        (void)fs_write(node, data, size);
    }
    return node;
}

static void make_numbered_name(const char *prefix, const char *suffix, int index, char *out) {
    strcpy(out, prefix);
    utoa((uint32_t)index, out + strlen(out), 10);
    strcat(out, suffix);
}

static fs_node_t create_text_demo(fs_node_t dir) {
    char name[16];
    static const char note[] =
        "RushOS note\n"
        "This is a demo text file from the Popeye Plasma desktop.\n";

    make_numbered_name("note", ".txt", g_new_text_id++, name);
    return create_file_in_dir(dir, name, note, (int)sizeof(note) - 1);
}

static fs_node_t create_image_demo(fs_node_t dir) {
    enum { DEMO_W = 32, DEMO_H = 24, HEADER_LEN = 13, FILE_LEN = HEADER_LEN + DEMO_W * DEMO_H * 3 };
    char name[16];
    uint8_t ppm[FILE_LEN];
    int pos = 0;

    ppm[pos++] = 'P'; ppm[pos++] = '6'; ppm[pos++] = '\n';
    ppm[pos++] = '3'; ppm[pos++] = '2'; ppm[pos++] = ' '; ppm[pos++] = '2'; ppm[pos++] = '4'; ppm[pos++] = '\n';
    ppm[pos++] = '2'; ppm[pos++] = '5'; ppm[pos++] = '5'; ppm[pos++] = '\n';

    for (int y = 0; y < DEMO_H; y++) {
        for (int x = 0; x < DEMO_W; x++) {
            uint8_t r = (uint8_t)(32 + x * 6);
            uint8_t g = (uint8_t)(108 + y * 4);
            uint8_t b = (uint8_t)(176 + (x + y) * 2);

            if (x > 8 && x < 23 && y > 7 && y < 17) {
                r = 236;
                g = 241;
                b = 248;
            }

            ppm[pos++] = r;
            ppm[pos++] = g;
            ppm[pos++] = b;
        }
    }

    make_numbered_name("img", ".ppm", g_new_image_id++, name);
    return create_file_in_dir(dir, name, ppm, pos);
}

static fs_node_t create_java_demo(fs_node_t dir) {
    char name[16];
    static const uint8_t hjar[] = {
        'H','J','A','R',1,
        0xFC, 24,
        'H','e','a','t','O','S',' ','m','i','n','i',' ','J','A','R',' ','l','a','u','n','c','h','e','d',
        0xFC, 1, '\n',
        0x10, 9,
        0x10, 4,
        0x68,
        0xFC, 8,
        '9',' ','*',' ','4',' ','=',' ',
        0xFE,
        0xB1
    };

    make_numbered_name("app", ".jar", g_new_jar_id++, name);
    return create_file_in_dir(dir, name, hjar, (int)sizeof(hjar));
}

static const char *default_title(AppId app) {
    if (app == APP_HOME) return "Settings";
    if (app == APP_FILES) return "File Manager";
    if (app == APP_BROWSER) return "Web Browser";
    if (app == APP_NET) return "Network";
    if (app == APP_JAVA) return "Java Console";
    if (app == APP_IMAGE) return "Image Viewer";
    return "Application";
}

static void window_assign_title(Window *w) {
    if ((w->app == APP_IMAGE || w->app == APP_JAVA) && w->node != 0) {
        strncpy(w->title, fs_get_name(w->node), sizeof(w->title) - 1);
        w->title[sizeof(w->title) - 1] = '\0';
        return;
    }

    strncpy(w->title, default_title(w->app), sizeof(w->title) - 1);
    w->title[sizeof(w->title) - 1] = '\0';
}

static void window_assign_geometry(Window *w, int slot) {
    w->x = 90 + slot * 26;
    w->y = 70 + slot * 22;
    w->width = 520;
    w->height = 320;

    if (w->app == APP_FILES) {
        w->width = 640;
        w->height = 410;
    } else if (w->app == APP_IMAGE) {
        w->width = 540;
        w->height = 410;
    } else if (w->app == APP_JAVA) {
        w->width = 560;
        w->height = 360;
    } else if (w->app == APP_BROWSER) {
        w->width = 620;
        w->height = 390;
    }
}

static Window *wm_focus_index(int index) {
    if (index < 0 || index >= g_window_count) return 0;

    Window temp = g_windows[index];
    for (int i = index; i < g_window_count - 1; i++) {
        g_windows[i] = g_windows[i + 1];
    }
    g_windows[g_window_count - 1] = temp;

    for (int i = 0; i < g_window_count; i++) g_windows[i].focused = false;
    g_windows[g_window_count - 1].focused = true;
    return &g_windows[g_window_count - 1];
}

static int wm_find_window(AppId app) {
    for (int i = 0; i < g_window_count; i++) {
        if (g_windows[i].app == app) return i;
    }
    return -1;
}

static Window *wm_open_window(AppId app, fs_node_t node) {
    int index = wm_find_window(app);

    if (index >= 0) {
        g_windows[index].node = node;
        window_assign_title(&g_windows[index]);
        return wm_focus_index(index);
    }

    if (g_window_count < MAX_WINDOWS) {
        index = g_window_count++;
    } else {
        for (int i = 0; i < MAX_WINDOWS - 1; i++) {
            g_windows[i] = g_windows[i + 1];
        }
        index = MAX_WINDOWS - 1;
    }

    Window *w = &g_windows[index];
    memset(w, 0, sizeof(*w));
    w->active = true;
    w->app = app;
    w->node = node;
    window_assign_geometry(w, index);
    window_assign_title(w);

    for (int i = 0; i < g_window_count; i++) g_windows[i].focused = false;
    w->focused = true;
    return w;
}

static void wm_close_window(int index) {
    if (index < 0 || index >= g_window_count) return;
    for (int i = index; i < g_window_count - 1; i++) {
        g_windows[i] = g_windows[i + 1];
    }
    g_window_count--;
    if (g_window_count > 0) {
        for (int i = 0; i < g_window_count; i++) g_windows[i].focused = false;
        g_windows[g_window_count - 1].focused = true;
    }
}

static bool app_is_open(AppId app) {
    return wm_find_window(app) >= 0;
}

static void change_directory(fs_node_t dir) {
    if (!fs_is_dir(dir)) return;
    g_files_dir = dir;
    g_selected_node = 0;
    set_status("Directory changed");
}

static void run_java_node(fs_node_t node) {
    g_java_node = node;
    if (!g_java_ready) {
        java_vm_init();
        g_java_ready = true;
    }

    if (java_vm_run_file(node, &g_java_result)) {
        set_status("Mini Java app executed");
    } else {
        set_status(g_java_result.error[0] ? g_java_result.error : "Java app failed");
    }

    (void)wm_open_window(APP_JAVA, node);
}

static void open_node(fs_node_t node) {
    const char *name;

    if (node == 0) {
        change_directory(0);
        (void)wm_open_window(APP_FILES, 0);
        return;
    }

    if (fs_is_dir(node)) {
        change_directory(node);
        (void)wm_open_window(APP_FILES, node);
        return;
    }

    name = fs_get_name(node);
    g_selected_node = node;

    if (is_image_name(name)) {
        g_image_node = node;
        if (image_get_info(node, &g_image_info)) {
            strcpy(g_image_note, "Previewing image file");
        } else {
            strcpy(g_image_note, "Unsupported or corrupt image");
            memset(&g_image_info, 0, sizeof(g_image_info));
        }
        (void)wm_open_window(APP_IMAGE, node);
        set_status("Image opened in viewer");
        return;
    }

    if (is_java_name(name)) {
        run_java_node(node);
        return;
    }

    set_status("File selected: no viewer yet");
}

static void draw_folder_icon(int x, int y, int size, uint32_t color) {
    fb_fill_rect(x + 2, y + 8, size - 4, size - 12, color);
    fb_fill_rect(x + 8, y + 3, size / 2, 10, blend_color(color, 0x00FFFFFF, 1, 4));
    fb_draw_rect(x + 2, y + 8, size - 4, size - 12, CLR_WINDOW_EDGE);
}

static void draw_picture_icon(int x, int y, int size, uint32_t color) {
    fb_fill_rect(x + 4, y + 4, size - 8, size - 8, color);
    fb_draw_rect(x + 4, y + 4, size - 8, size - 8, CLR_WINDOW_EDGE);
    fb_fill_rect(x + 9, y + 10, size - 18, size - 18, 0x00E8F3FA);
    fb_draw_line(x + 12, y + size - 12, x + size / 2, y + size / 2, 0x0048A2CE);
    fb_draw_line(x + size / 2, y + size / 2, x + size - 14, y + size - 14, 0x0075D16F);
    fb_fill_circle(x + size - 16, y + 14, 4, 0x00F4D45B);
}

static void draw_java_icon(int x, int y, int size, uint32_t color) {
    fb_fill_rect(x + 8, y + 14, size - 16, size - 18, color);
    fb_draw_rect(x + 8, y + 14, size - 16, size - 18, CLR_WINDOW_EDGE);
    fb_draw_line(x + 12, y + 10, x + size - 12, y + 10, 0x00FFFFFF);
    fb_draw_line(x + 14, y + 7, x + size - 18, y + 4, 0x00FFFFFF);
    fb_draw_line(x + size - 14, y + 10, x + size - 8, y + 16, 0x00FFFFFF);
}

static void draw_desktop_icon(int x, int y, const char *label, AppId app) {
    bool hover = hit(g_mouse_x, g_mouse_y, x - 6, y - 6, 72, 84);
    uint32_t frame = hover ? CLR_ACCENT : 0x004D6678;

    fb_fill_rect(x - 4, y - 4, 58, 64, hover ? 0x002A4253 : 0x001D3447);
    fb_draw_rect(x - 4, y - 4, 58, 64, frame);

    if (app == APP_HOME || app == APP_FILES) {
        draw_folder_icon(x + 8, y + 10, 34, app == APP_HOME ? 0x00E1B85C : CLR_FOLDER);
    } else if (app == APP_JAVA) {
        draw_java_icon(x + 8, y + 8, 34, CLR_JAVA);
    }

    fb_draw_string(x - 2, y + 66, label, CLR_TEXT, 0xFF000000);
}

static void draw_wallpaper(void) {
    draw_gradient_rect(0, 0, fb_width, fb_height, 0x003BC7C7, 0x001580B6);
    fb_fill_circle(fb_width / 2, fb_height + 220, 620, 0x006BC9D8);
    fb_fill_circle(fb_width / 2 + 160, fb_height + 250, 520, 0x0046AAD1);
    fb_fill_circle(-90, fb_height + 170, 430, 0x002E94C7);
    fb_draw_string(fb_width - 140, fb_height - 24, "RushOS XFCE", 0x00EAF7FF, 0xFF000000);

    draw_desktop_icon(22, 74, "Home", APP_HOME);
    draw_desktop_icon(22, 160, "Files", APP_FILES);
    draw_desktop_icon(22, 246, "Java", APP_JAVA);
}

static void draw_panel(void) {
    draw_gradient_rect(0, 0, fb_width, PANEL_H, CLR_PANEL_TOP, CLR_PANEL_BOT);
    fb_draw_line(0, PANEL_H - 1, fb_width, PANEL_H - 1, CLR_PANEL_EDGE);

    fb_fill_rect(4, 4, 118, 24, g_menu_open ? CLR_ACCENT : CLR_MENU_ALT);
    fb_draw_rect(4, 4, 118, 24, CLR_PANEL_EDGE);
    fb_draw_string(14, 8, "Applications", CLR_TEXT, 0xFF000000);

    int task_x = 132;
    for (int i = 0; i < g_window_count; i++) {
        uint32_t bg = g_windows[i].focused ? 0x00404850 : 0x00323336;
        fb_fill_rect(task_x, 4, 104, 24, bg);
        fb_draw_rect(task_x, 4, 104, 24, CLR_PANEL_EDGE);
        fb_draw_string(task_x + 8, 8, g_windows[i].title, CLR_TEXT, 0xFF000000);
        task_x += 110;
    }

    int tray_x = fb_width - 158;
    fb_fill_rect(tray_x, 7, 10, 10, CLR_GOOD);
    fb_fill_rect(tray_x + 16, 7, 10, 10, CLR_WARN);
    fb_fill_rect(tray_x + 32, 7, 10, 10, CLR_IMAGE);
    fb_draw_string(fb_width - 100, 8, "28%  1:16", CLR_TEXT, 0xFF000000);
}

static void draw_menu(void) {
    if (!g_menu_open) return;

    int menu_x = 6;
    int menu_y = PANEL_H + 4;
    int menu_w = 332;
    int menu_h = 246;

    fb_fill_rect(menu_x + 6, menu_y + 6, menu_w, menu_h, CLR_SHADOW);
    fb_fill_rect(menu_x, menu_y, menu_w, menu_h, CLR_MENU_BG);
    fb_draw_rect(menu_x, menu_y, menu_w, menu_h, CLR_WINDOW_EDGE);
    fb_fill_rect(menu_x, menu_y, 108, menu_h, CLR_MENU_SIDE);

    fb_draw_string(menu_x + 14, menu_y + 14, "Accessories", CLR_TEXT, 0xFF000000);
    fb_draw_string(menu_x + 14, menu_y + 46, "Internet", CLR_TEXT_DIM, 0xFF000000);
    fb_draw_string(menu_x + 14, menu_y + 78, "System", CLR_TEXT_DIM, 0xFF000000);
    fb_draw_string(menu_x + 14, menu_y + 110, "Graphics", CLR_TEXT_DIM, 0xFF000000);

    g_menu_hover = -1;
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        int row_y = menu_y + 18 + i * 42;
        bool hover = hit(g_mouse_x, g_mouse_y, menu_x + 118, row_y - 4, 204, 34);
        if (hover) g_menu_hover = i;

        fb_fill_rect(menu_x + 118, row_y - 4, 204, 34, hover ? CLR_MENU_SEL : CLR_MENU_BG);
        fb_fill_rect(menu_x + 126, row_y + 4, 18, 18, g_menu_items[i].accent);
        fb_draw_rect(menu_x + 126, row_y + 4, 18, 18, CLR_WINDOW_EDGE);
        fb_draw_string(menu_x + 154, row_y, g_menu_items[i].label, CLR_TEXT, 0xFF000000);
        fb_draw_string(menu_x + 154, row_y + 14, g_menu_items[i].hint, CLR_TEXT_FAINT, 0xFF000000);
    }
}

static void draw_dock(void) {
    int dock_x = (fb_width - DOCK_W) / 2;
    int dock_y = fb_height - 58;
    AppId apps[5] = { APP_HOME, APP_FILES, APP_BROWSER, APP_JAVA, APP_NET };
    const uint32_t colors[5] = { CLR_ACCENT, CLR_FOLDER, CLR_IMAGE, CLR_JAVA, CLR_WARN };

    fb_fill_rect(dock_x + 6, dock_y + 6, DOCK_W, DOCK_H, CLR_SHADOW);
    fb_fill_rect(dock_x, dock_y, DOCK_W, DOCK_H, 0x00313639);
    fb_draw_rect(dock_x, dock_y, DOCK_W, DOCK_H, CLR_WINDOW_EDGE);

    for (int i = 0; i < 5; i++) {
        int bx = dock_x + 10 + i * 52;
        bool hover = hit(g_mouse_x, g_mouse_y, bx, dock_y + 6, 40, 32);
        fb_fill_rect(bx, dock_y + 6, 40, 32, hover ? 0x00414A52 : 0x00373D44);
        fb_draw_rect(bx, dock_y + 6, 40, 32, CLR_WINDOW_EDGE);
        fb_fill_rect(bx + 8, dock_y + 14, 24, 16, colors[i]);
        if (app_is_open(apps[i])) {
            fb_fill_rect(bx + 12, dock_y + 36, 16, 3, CLR_TEXT);
        }
    }
}

static void draw_window_frame(const Window &w) {
    uint32_t title_top = w.focused ? CLR_WINDOW_TOP : CLR_WINDOW_TOPF;
    uint32_t title_bot = w.focused ? 0x003A3D41 : 0x00303336;

    fb_fill_rect(w.x + 8, w.y + 8, w.width, w.height, CLR_SHADOW);
    fb_fill_rect(w.x, w.y, w.width, w.height, CLR_WINDOW_BG);
    fb_draw_rect(w.x, w.y, w.width, w.height, CLR_WINDOW_EDGE);
    draw_gradient_rect(w.x, w.y, w.width, 30, title_top, title_bot);
    fb_draw_line(w.x, w.y + 30, w.x + w.width, w.y + 30, CLR_WINDOW_EDGE);
    fb_draw_string(w.x + 12, w.y + 8, w.title, CLR_TEXT, 0xFF000000);

    fb_fill_rect(w.x + w.width - 68, w.y + 7, 14, 14, 0x00D3A94B);
    fb_fill_rect(w.x + w.width - 46, w.y + 7, 14, 14, 0x0080C56B);
    fb_fill_rect(w.x + w.width - 24, w.y + 7, 14, 14, 0x00CA5757);
    fb_draw_rect(w.x + w.width - 24, w.y + 7, 14, 14, CLR_WINDOW_EDGE);
}

static void draw_files(Window &w) {
    int cx = w.x + 2;
    int cy = w.y + 32;
    int cw = w.width - 4;
    int ch = w.height - 34;
    char path[64];

    fb_fill_rect(cx, cy, cw, ch, CLR_WINDOW_IN);
    fb_fill_rect(cx, cy, 150, ch, 0x0034383C);
    fb_draw_line(cx + 150, cy, cx + 150, cy + ch, CLR_WINDOW_EDGE);

    fb_fill_rect(cx + 150, cy, cw - 150, 38, 0x00363A3E);
    fb_draw_line(cx + 150, cy + 38, cx + cw, cy + 38, CLR_WINDOW_EDGE);

    fb_draw_string(cx + 12, cy + 16, "Places", CLR_TEXT, 0xFF000000);
    fb_draw_string(cx + 12, cy + 48, "/", CLR_TEXT_DIM, 0xFF000000);
    fb_draw_string(cx + 12, cy + 76, "/home", CLR_TEXT_DIM, 0xFF000000);
    fb_draw_string(cx + 12, cy + 104, "/java", CLR_TEXT_DIM, 0xFF000000);

    fb_fill_rect(cx + 162, cy + 8, 74, 22, CLR_ACCENT);
    fb_fill_rect(cx + 244, cy + 8, 74, 22, CLR_IMAGE);
    fb_fill_rect(cx + 326, cy + 8, 74, 22, CLR_JAVA);
    fb_draw_string(cx + 179, cy + 12, "+ TXT", CLR_TEXT, 0xFF000000);
    fb_draw_string(cx + 260, cy + 12, "+ IMG", CLR_TEXT, 0xFF000000);
    fb_draw_string(cx + 343, cy + 12, "+ JAR", CLR_TEXT, 0xFF000000);

    build_node_path(g_files_dir, path, sizeof(path));
    fb_draw_string(cx + 418, cy + 12, path, CLR_TEXT_DIM, 0xFF000000);

    int fx = cx + 162;
    int fy = cy + 54;
    int count = 0;
    fs_node_t iter;

    if (fs_list_begin(g_files_dir, &iter)) {
        fs_node_t child;
        while (fs_list_next(&iter, &child)) {
            bool hovered = hit(g_mouse_x, g_mouse_y, fx, fy, 92, 92);
            bool selected = g_selected_node == child;
            uint32_t tile = selected ? 0x00455C6C : (hovered ? 0x0040464A : 0x00373C40);
            const char *name = fs_get_name(child);

            fb_fill_rect(fx, fy, 92, 92, tile);
            fb_draw_rect(fx, fy, 92, 92, selected ? CLR_ACCENT : CLR_WINDOW_EDGE);

            if (fs_is_dir(child)) {
                draw_folder_icon(fx + 25, fy + 14, 42, CLR_FOLDER);
            } else if (is_image_name(name)) {
                draw_picture_icon(fx + 24, fy + 14, 42, CLR_IMAGE);
            } else if (is_java_name(name)) {
                draw_java_icon(fx + 24, fy + 12, 42, CLR_JAVA);
            } else {
                fb_fill_rect(fx + 28, fy + 14, 36, 46, 0x00D8DDE2);
                fb_draw_rect(fx + 28, fy + 14, 36, 46, CLR_WINDOW_EDGE);
            }

            fb_fill_rect(fx + 70, fy + 6, 14, 14, CLR_ERROR);
            fb_draw_string(fx + 74, fy + 5, "x", CLR_TEXT, 0xFF000000);
            fb_draw_string(fx + 10, fy + 68, name, CLR_TEXT, 0xFF000000);
            fb_draw_string(fx + 10, fy + 80, fs_is_dir(child) ? "folder" : "file", CLR_TEXT_FAINT, 0xFF000000);

            count++;
            fx += 100;
            if ((count % 4) == 0) {
                fx = cx + 162;
                fy += 100;
            }
        }
    } else {
        fb_draw_string(cx + 176, cy + 72, "Directory is empty", CLR_TEXT_DIM, 0xFF000000);
    }

    fb_fill_rect(cx + 8, cy + ch - 28, cw - 16, 20, 0x00323639);
    fb_draw_string(cx + 14, cy + ch - 24, g_status_line, CLR_TEXT_DIM, 0xFF000000);
}

static void draw_home(Window &w) {
    int x = w.x + 18;
    int y = w.y + 52;

    fb_draw_string(x, y, "RushOS Desktop Settings", CLR_TEXT, 0xFF000000); y += 32;
    fb_draw_string(x, y, "Style:    XFCE-inspired shell with top panel + dock", CLR_TEXT_DIM, 0xFF000000); y += 20;
    fb_draw_string(x, y, "Graphics: virtio-gpu / BGA via framebuffer backend", CLR_TEXT_DIM, 0xFF000000); y += 20;
    fb_draw_string(x, y, "Files:    BMP/PPM image preview in the desktop viewer", CLR_TEXT_DIM, 0xFF000000); y += 20;
    fb_draw_string(x, y, "Java:     HeatOS mini VM supports custom .jar wrappers", CLR_TEXT_DIM, 0xFF000000); y += 20;
    fb_draw_string(x, y, "Limit:    Real JVM .jar files need ZIP + class loading", CLR_WARN, 0xFF000000); y += 28;
    draw_text_block(x, y, w.width - 40,
        "Open File Manager to inspect /home/sample.ppm and /java/hello.jar.\n"
        "The Java console can execute HeatOS HJAR files and will report why\n"
        "standard desktop Java archives are not supported yet.",
        CLR_TEXT_DIM);
}

static void draw_browser(Window &w) {
    int x = w.x + 12;
    int y = w.y + 42;
    int width = w.width - 24;
    int body_h = w.height - 54;

    fb_fill_rect(x, y, width, 34, 0x00323639);
    fb_draw_rect(x, y, width, 34, CLR_WINDOW_EDGE);
    fb_fill_rect(x + 48, y + 7, width - 60, 20, 0x00E7EDF2);
    fb_draw_string(x + 58, y + 11, "https://rushos.local/start", 0x00303639, 0xFF000000);

    fb_fill_rect(x, y + 42, width, body_h - 42, 0x00F3F7FA);
    fb_draw_rect(x, y + 42, width, body_h - 42, CLR_WINDOW_EDGE);
    fb_draw_string(x + 18, y + 64, "Nugget Browser", 0x0029343A, 0xFF000000);
    draw_text_block(x + 18, y + 92, width - 40,
        "This is still a mock browser shell. It is styled closer to XFCE\n"
        "but it is not a real HTML/CSS engine yet. The display backend work\n"
        "does make that future step less painful because rendering is now\n"
        "decoupled from the old direct BGA path.",
        0x00414C55);
}

static void draw_net(Window &w) {
    int x = w.x + 18;
    int y = w.y + 50;
    fb_draw_string(x, y, "System Status", CLR_TEXT, 0xFF000000); y += 28;
    fb_draw_string(x, y, "Display backend: framebuffer -> virtio-gpu fallback BGA", CLR_TEXT_DIM, 0xFF000000); y += 20;
    fb_draw_string(x, y, "Mouse driver: rewritten PS/2 reset + packet queue path", CLR_TEXT_DIM, 0xFF000000); y += 20;
    fb_draw_string(x, y, "Desktop shell: panel, whisker menu, dock, file viewers", CLR_TEXT_DIM, 0xFF000000); y += 20;
    fb_draw_string(x, y, "Java runtime: HeatOS mini bytecode VM", CLR_TEXT_DIM, 0xFF000000); y += 28;
    draw_text_block(x, y, w.width - 36,
        "The remaining gap versus real XFCE/KDE is not the pixel output anymore.\n"
        "It is the missing process model, full widget toolkit, icon theme stack,\n"
        "real font system, and standard application runtimes.",
        CLR_TEXT_DIM);
}

static void draw_java(Window &w) {
    int x = w.x + 14;
    int y = w.y + 42;
    int width = w.width - 28;
    char path[64];

    fb_fill_rect(x, y, width, 28, 0x0033373B);
    fb_draw_rect(x, y, width, 28, CLR_WINDOW_EDGE);
    fb_draw_string(x + 10, y + 8, "HeatOS Mini Java Console", CLR_TEXT, 0xFF000000);

    if (g_java_node != 0) {
        build_node_path(g_java_node, path, sizeof(path));
        fb_draw_string(x + 10, y + 40, path, CLR_TEXT_DIM, 0xFF000000);
    } else {
        fb_draw_string(x + 10, y + 40, "Open a .jar file from File Manager", CLR_TEXT_DIM, 0xFF000000);
    }

    fb_fill_rect(x, y + 66, width, w.height - 118, 0x001E2225);
    fb_draw_rect(x, y + 66, width, w.height - 118, CLR_WINDOW_EDGE);

    if (g_java_result.success) {
        draw_text_block(x + 10, y + 76, width - 20, g_java_result.output, CLR_TEXT);
    } else if (g_java_result.error[0]) {
        draw_text_block(x + 10, y + 76, width - 20, g_java_result.error, CLR_ERROR);
    } else {
        draw_text_block(x + 10, y + 76, width - 20,
            "No Java app executed yet.\n"
            "HeatOS supports demo HJAR files and raw mini-bytecode programs.\n"
            "Real desktop Java .jar files are ZIP archives of .class files and\n"
            "need a much larger JVM implementation than this kernel currently has.",
            CLR_TEXT_DIM);
    }
}

static void draw_image(Window &w) {
    int x = w.x + 14;
    int y = w.y + 42;
    int width = w.width - 28;
    int height = w.height - 56;
    char info[64];

    fb_fill_rect(x, y, width, 28, 0x0033373B);
    fb_draw_rect(x, y, width, 28, CLR_WINDOW_EDGE);

    if (g_image_node != 0 && image_get_info(g_image_node, &g_image_info)) {
        strcpy(info, image_format_name(g_image_info.format));
        strcat(info, " ");
        utoa((uint32_t)g_image_info.width, info + strlen(info), 10);
        strcat(info, "x");
        utoa((uint32_t)g_image_info.height, info + strlen(info), 10);
        fb_draw_string(x + 10, y + 8, info, CLR_TEXT, 0xFF000000);
    } else {
        fb_draw_string(x + 10, y + 8, g_image_note, CLR_TEXT, 0xFF000000);
    }

    fb_fill_rect(x, y + 38, width, height - 38, 0x00242D35);
    fb_draw_rect(x, y + 38, width, height - 38, CLR_WINDOW_EDGE);

    if (g_image_node != 0 && image_draw_file(g_image_node, x + 10, y + 48, width - 20, height - 58, 0x00242D35)) {
        return;
    }

    draw_text_block(x + 18, y + 64, width - 36,
        "Open a BMP or PPM file from the file manager to preview it here.",
        CLR_TEXT_DIM);
}

static void draw_windows(void) {
    for (int i = 0; i < g_window_count; i++) {
        Window &w = g_windows[i];
        draw_window_frame(w);

        if (w.app == APP_HOME) draw_home(w);
        else if (w.app == APP_FILES) draw_files(w);
        else if (w.app == APP_BROWSER) draw_browser(w);
        else if (w.app == APP_NET) draw_net(w);
        else if (w.app == APP_JAVA) draw_java(w);
        else if (w.app == APP_IMAGE) draw_image(w);
    }
}

static void draw_cursor(void) {
    int mx = g_mouse_x;
    int my = g_mouse_y;

    fb_draw_line(mx + 1, my + 2, mx + 12, my + 14, CLR_SHADOW);
    fb_draw_line(mx, my, mx + 16, my + 10, CLR_TEXT);
    fb_draw_line(mx, my, mx + 10, my + 16, CLR_TEXT);
    fb_draw_line(mx + 16, my + 10, mx + 10, my + 16, CLR_TEXT);
    for (int y = 1; y < 14; y++) {
        for (int x = 1; x < 12; x++) {
            if (x + y < 16 && x < y + 9) {
                fb_put_pixel(mx + x, my + y, 0x00000000);
            }
        }
    }
}

static void handle_desktop_launchers(bool clicked) {
    if (!clicked) return;

    if (hit(g_mouse_x, g_mouse_y, 16, 68, 72, 84)) {
        (void)wm_open_window(APP_HOME, 0);
    } else if (hit(g_mouse_x, g_mouse_y, 16, 154, 72, 84)) {
        (void)wm_open_window(APP_FILES, g_files_dir);
    } else if (hit(g_mouse_x, g_mouse_y, 16, 240, 72, 84)) {
        (void)wm_open_window(APP_JAVA, g_java_node);
    }
}

static void handle_dock_click(bool clicked) {
    AppId apps[5] = { APP_HOME, APP_FILES, APP_BROWSER, APP_JAVA, APP_NET };
    int dock_x = (fb_width - DOCK_W) / 2;
    int dock_y = fb_height - 58;

    if (!clicked) return;

    for (int i = 0; i < 5; i++) {
        int bx = dock_x + 10 + i * 52;
        if (hit(g_mouse_x, g_mouse_y, bx, dock_y + 6, 40, 32)) {
            (void)wm_open_window(apps[i], 0);
            g_menu_open = false;
            return;
        }
    }
}

static void handle_menu_click(bool clicked) {
    if (!clicked) return;

    if (hit(g_mouse_x, g_mouse_y, 4, 4, 118, 24)) {
        g_menu_open = !g_menu_open;
        return;
    }

    if (g_menu_open) {
        int menu_x = 6;
        int menu_y = PANEL_H + 4;
        int menu_w = 332;
        int menu_h = 246;

        if (hit(g_mouse_x, g_mouse_y, menu_x, menu_y, menu_w, menu_h)) {
            if (g_menu_hover >= 0) {
                (void)wm_open_window(g_menu_items[g_menu_hover].app, 0);
            }
            g_menu_open = false;
            return;
        }

        g_menu_open = false;
    }
}

static void handle_panel_tasks(bool clicked) {
    int task_x = 132;
    if (!clicked) return;

    for (int i = 0; i < g_window_count; i++) {
        if (hit(g_mouse_x, g_mouse_y, task_x, 4, 104, 24)) {
            (void)wm_focus_index(i);
            return;
        }
        task_x += 110;
    }
}

static void handle_files_click(Window &w, bool clicked) {
    int cx = w.x + 2;
    int cy = w.y + 32;
    int ch = w.height - 34;

    if (!clicked) return;

    if (hit(g_mouse_x, g_mouse_y, cx + 12, cy + 42, 60, 18)) {
        change_directory(0);
        return;
    }
    if (hit(g_mouse_x, g_mouse_y, cx + 12, cy + 70, 72, 18)) {
        fs_node_t home = fs_resolve("/home");
        if (home != 0) change_directory(home);
        return;
    }
    if (hit(g_mouse_x, g_mouse_y, cx + 12, cy + 98, 72, 18)) {
        fs_node_t java = fs_resolve("/java");
        if (java != 0) change_directory(java);
        return;
    }

    if (hit(g_mouse_x, g_mouse_y, cx + 162, cy + 8, 74, 22)) {
        if (create_text_demo(g_files_dir) != 0) set_status("Text file created");
        else set_status("Failed to create text file");
        return;
    }
    if (hit(g_mouse_x, g_mouse_y, cx + 244, cy + 8, 74, 22)) {
        if (create_image_demo(g_files_dir) != 0) set_status("PPM image created");
        else set_status("Failed to create image");
        return;
    }
    if (hit(g_mouse_x, g_mouse_y, cx + 326, cy + 8, 74, 22)) {
        if (create_java_demo(g_files_dir) != 0) set_status("Mini .jar created");
        else set_status("Failed to create Java app");
        return;
    }

    int fx = cx + 162;
    int fy = cy + 54;
    int count = 0;
    fs_node_t iter;
    if (fs_list_begin(g_files_dir, &iter)) {
        fs_node_t child;
        while (fs_list_next(&iter, &child)) {
            if (hit(g_mouse_x, g_mouse_y, fx + 70, fy + 6, 14, 14)) {
                if (fs_delete(child)) {
                    if (g_selected_node == child) g_selected_node = 0;
                    set_status("Entry deleted");
                } else {
                    set_status("Delete failed (folder not empty?)");
                }
                return;
            }

            if (hit(g_mouse_x, g_mouse_y, fx, fy, 92, 92)) {
                g_selected_node = child;
                open_node(child);
                return;
            }

            count++;
            fx += 100;
            if ((count % 4) == 0) {
                fx = cx + 162;
                fy += 100;
            }
            if (fy > cy + ch - 100) break;
        }
    }
}

static void clamp_window(Window *w) {
    if (!w) return;
    if (w->x < 8) w->x = 8;
    if (w->y < PANEL_H + 6) w->y = PANEL_H + 6;
    if (w->x + w->width > fb_width - 8) w->x = fb_width - 8 - w->width;
    if (w->y + w->height > fb_height - 66) w->y = fb_height - 66 - w->height;
}

static void handle_clicks(void) {
    bool clicked = g_mouse_ldown && !g_was_ldown;

    if (g_mouse_ldown && g_drag_win) {
        g_drag_win->x = g_mouse_x - g_drag_offset_x;
        g_drag_win->y = g_mouse_y - g_drag_offset_y;
        clamp_window(g_drag_win);
        return;
    }
    g_drag_win = 0;

    handle_menu_click(clicked);
    handle_panel_tasks(clicked);
    handle_desktop_launchers(clicked);
    handle_dock_click(clicked);

    if (!clicked) return;

    for (int i = g_window_count - 1; i >= 0; i--) {
        if (!hit(g_mouse_x, g_mouse_y, g_windows[i].x, g_windows[i].y, g_windows[i].width, g_windows[i].height)) {
            continue;
        }

        Window *w = wm_focus_index(i);
        if (!w) return;

        if (hit(g_mouse_x, g_mouse_y, w->x + w->width - 24, w->y + 7, 14, 14)) {
            wm_close_window(g_window_count - 1);
            return;
        }

        if (hit(g_mouse_x, g_mouse_y, w->x, w->y, w->width, 30)) {
            g_drag_win = &g_windows[g_window_count - 1];
            g_drag_offset_x = g_mouse_x - w->x;
            g_drag_offset_y = g_mouse_y - w->y;
            return;
        }

        if (w->app == APP_FILES) {
            handle_files_click(*w, clicked);
        }
        return;
    }
}

bool popeye_plasma_desktop_run(bool java_enabled) {
    (void)java_enabled;
    if (!fb_init()) return false;

    mouse_set_reporting(true);
    java_vm_init();
    g_java_ready = true;

    g_exit = false;
    g_menu_open = false;
    g_menu_hover = -1;
    g_drag_win = 0;
    g_mouse_ldown = false;
    g_was_ldown = false;
    g_window_count = 0;
    g_selected_node = 0;
    g_image_node = 0;
    g_java_node = 0;
    g_status_line[0] = '\0';
    strcpy(g_image_note, "Open an image from File Manager");
    memset(&g_image_info, 0, sizeof(g_image_info));
    memset(&g_java_result, 0, sizeof(g_java_result));

    g_files_dir = fs_resolve("/home");
    if (g_files_dir == 0) g_files_dir = 0;

    (void)wm_open_window(APP_FILES, g_files_dir);
    (void)wm_open_window(APP_HOME, 0);
    set_status("Desktop ready: open /home/sample.ppm or /java/hello.jar");

    while (keyboard_poll() != 0) {}
    while (mouse_poll(0)) {}

    while (!g_exit) {
        for (int i = 0; i < 100; i++) {
            int key = keyboard_poll();
            if (key == 0) break;
            if (key == KEY_F2 || key == KEY_ESCAPE) g_exit = true;
            if (key == 'f' || key == 'F') (void)wm_open_window(APP_FILES, g_files_dir);
            if (key == 'j' || key == 'J') (void)wm_open_window(APP_JAVA, g_java_node);
        }

        for (int i = 0; i < 128; i++) {
            mouse_packet_t pkt;
            if (!mouse_poll(&pkt)) break;
            g_mouse_x += pkt.dx;
            g_mouse_y -= pkt.dy;
            if (g_mouse_x < 0) g_mouse_x = 0;
            if (g_mouse_x > fb_width - 1) g_mouse_x = fb_width - 1;
            if (g_mouse_y < 0) g_mouse_y = 0;
            if (g_mouse_y > fb_height - 1) g_mouse_y = fb_height - 1;
            g_mouse_ldown = pkt.left;
        }

        handle_clicks();
        g_was_ldown = g_mouse_ldown;

        draw_wallpaper();
        draw_windows();
        draw_panel();
        draw_menu();
        draw_dock();
        draw_cursor();
        fb_swap();
    }

    mouse_set_reporting(false);
    fb_shutdown();
    return true;
}

