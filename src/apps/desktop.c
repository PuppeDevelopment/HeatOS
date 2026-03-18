#include "desktop.h"

#include "graphics.h"
#include "keyboard.h"
#include "mouse.h"
#include "string.h"
#include "types.h"
#include "vga.h"

#define DESKTOP_TOP 1
#define DESKTOP_BOTTOM 21
#define DOCK_TOP 22
#define DOCK_HEIGHT 3

#define WINDOW_COUNT 4

#define MENU_X 1
#define MENU_Y 1
#define MENU_W 28
#define MENU_H 7

#define DOCK_SLOT_W 14
#define DOCK_START_X ((VGA_WIDTH - (WINDOW_COUNT * DOCK_SLOT_W)) / 2)

typedef struct {
    const char *id;
    const char *title;
    int x;
    int y;
    int w;
    int h;
    bool visible;
} desk_window_t;

static desk_window_t g_windows[WINDOW_COUNT] = {
    {"term", "Terminal", 4, 4, 38, 12, true},
    {"notes", "Notes", 18, 6, 34, 11, true},
    {"about", "About HeatOS", 28, 5, 44, 12, true},
    {"retro", "Retro Board", 12, 8, 36, 10, false}
};

static const char *g_dock_items[WINDOW_COUNT] = {
    "Terminal",
    "Notes",
    "About",
    "Retro"
};

static int g_zorder[WINDOW_COUNT] = {0, 1, 3, 2};
static int g_active_win = 2;
static int g_dock_index = 0;

static bool g_menu_open = false;
static bool g_drag_active = false;
static int g_drag_window = -1;

static bool g_mouse_ok = false;
static int g_mouse_x = VGA_WIDTH / 2;
static int g_mouse_y = VGA_HEIGHT / 2;
static bool g_left_prev = false;

static uint32_t g_frame_counter = 0;

static int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int dock_slot_x(int idx) {
    return DOCK_START_X + (idx * DOCK_SLOT_W);
}

static void clamp_window(desk_window_t *w) {
    int min_x = 1;
    int max_x = VGA_WIDTH - w->w - 1;
    int min_y = DESKTOP_TOP + 1;
    int max_y = DESKTOP_BOTTOM - w->h;

    if (max_x < min_x) max_x = min_x;
    if (max_y < min_y) max_y = min_y;

    w->x = clampi(w->x, min_x, max_x);
    w->y = clampi(w->y, min_y, max_y);
}

static void bring_to_front(int win_idx) {
    if (win_idx < 0 || win_idx >= WINDOW_COUNT) return;

    int pos = -1;
    for (int i = 0; i < WINDOW_COUNT; i++) {
        if (g_zorder[i] == win_idx) {
            pos = i;
            break;
        }
    }
    if (pos < 0) return;

    for (int i = pos; i < WINDOW_COUNT - 1; i++) {
        g_zorder[i] = g_zorder[i + 1];
    }
    g_zorder[WINDOW_COUNT - 1] = win_idx;
    g_active_win = win_idx;
}

static int window_from_dock(int idx) {
    if (idx < 0 || idx >= WINDOW_COUNT) return 0;
    return idx;
}

static void draw_wallpaper(void) {
    for (int r = DESKTOP_TOP; r <= DESKTOP_BOTTOM; r++) {
        uint8_t attr = (r & 1) ? VGA_ATTR(VGA_CYAN, VGA_BLUE)
                               : VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLUE);

        for (int c = 0; c < VGA_WIDTH; c++) {
            char ch = ((c + r) % 11 == 0) ? '.' : ' ';
            gfx_put_cell(r, c, ch, attr);
        }
    }

    gfx_write_text(3, 3, "HeatOS Desktop", VGA_ATTR(VGA_WHITE, VGA_BLUE));
    gfx_write_text(4, 3, "Simple. Educational. Nostalgic.", VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLUE));
}

static void draw_menu_panel(void) {
    if (!g_menu_open) return;

    uint8_t panel_attr = VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY);
    gfx_fill_rect(MENU_Y, MENU_X, MENU_H, MENU_W, ' ', panel_attr);
    gfx_write_text(MENU_Y + 1, MENU_X + 2, "About Desktop", panel_attr);
    gfx_write_text(MENU_Y + 2, MENU_X + 2, "Reset Layout", panel_attr);
    gfx_write_text(MENU_Y + 3, MENU_X + 2, "Hide Active Window", panel_attr);
    gfx_write_text(MENU_Y + 4, MENU_X + 2, "Close Menu", panel_attr);
}

static void draw_menu_bar(void) {
    char clock_buf[6];
    int mins = (int)((g_frame_counter / 50u) % 60u);
    int hours = 9 + (int)((g_frame_counter / (50u * 60u)) % 9u);

    gfx_fill_row(0, ' ', VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY));
    gfx_write_text(0, 2, "HeatOS", VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY));
    gfx_write_text(0, 10, "File  Edit  View  Window  Help", VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY));

    if (gfx_backend() == GFX_BACKEND_VIRTIO_TEXT) {
        gfx_write_text(0, 45, "Backend: virtio API", VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY));
    } else {
        gfx_write_text(0, 48, "Backend: VGA API", VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY));
    }

    clock_buf[0] = (char)('0' + (hours / 10));
    clock_buf[1] = (char)('0' + (hours % 10));
    clock_buf[2] = ':';
    clock_buf[3] = (char)('0' + (mins / 10));
    clock_buf[4] = (char)('0' + (mins % 10));
    clock_buf[5] = '\0';
    gfx_write_text(0, 74, clock_buf, VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY));

    draw_menu_panel();
}

static void draw_window_content(const desk_window_t *w) {
    int cx = w->x + 2;
    int cy = w->y + 2;
    uint8_t content_attr = VGA_ATTR(VGA_BLACK, VGA_WHITE);

    if (strcmp(w->id, "term") == 0) {
        gfx_write_text(cy + 0, cx, "$ HeatOS desktop terminal", content_attr);
        gfx_write_text(cy + 1, cx, "$ Click title bar and drag windows", content_attr);
        gfx_write_text(cy + 2, cx, "$ Press Esc to return to shell", content_attr);
    } else if (strcmp(w->id, "notes") == 0) {
        gfx_write_text(cy + 0, cx, "Notes", content_attr);
        gfx_write_text(cy + 1, cx, "- Build from scratch", content_attr);
        gfx_write_text(cy + 2, cx, "- Keep systems understandable", content_attr);
        gfx_write_text(cy + 3, cx, "- Learn by reading code", content_attr);
    } else if (strcmp(w->id, "about") == 0) {
        gfx_write_text(cy + 0, cx, "Aesthetic: modern macOS-inspired", content_attr);
        gfx_write_text(cy + 1, cx, "Philosophy: simple + educational", content_attr);
        gfx_write_text(cy + 2, cx, "Nostalgic desktop shell for learning", content_attr);
        gfx_write_text(cy + 3, cx, "Not Linux, not production, just craft", content_attr);
    } else {
        gfx_write_text(cy + 0, cx, "+--------------------------+", content_attr);
        gfx_write_text(cy + 1, cx, "| Retro Board             |", content_attr);
        gfx_write_text(cy + 2, cx, "| o o o o o o             |", content_attr);
        gfx_write_text(cy + 3, cx, "| Keep hacking            |", content_attr);
        gfx_write_text(cy + 4, cx, "+--------------------------+", content_attr);
    }
}

static void draw_window_frame(const desk_window_t *w, bool active) {
    if (!w->visible) return;

    uint8_t shadow_attr = VGA_ATTR(VGA_DARK_GRAY, VGA_BLACK);
    uint8_t title_attr = active ? VGA_ATTR(VGA_WHITE, VGA_BLUE)
                                : VGA_ATTR(VGA_LIGHT_GRAY, VGA_DARK_GRAY);
    uint8_t frame_attr = VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY);
    uint8_t content_attr = VGA_ATTR(VGA_BLACK, VGA_WHITE);

    for (int r = w->y + 1; r <= w->y + w->h && r < VGA_HEIGHT; r++) {
        if (w->x + w->w < VGA_WIDTH) {
            gfx_put_cell(r, w->x + w->w, ' ', shadow_attr);
        }
    }
    if (w->y + w->h < VGA_HEIGHT) {
        for (int c = w->x + 1; c <= w->x + w->w && c < VGA_WIDTH; c++) {
            gfx_put_cell(w->y + w->h, c, ' ', shadow_attr);
        }
    }

    gfx_fill_rect(w->y, w->x, 1, w->w, ' ', title_attr);
    gfx_put_cell(w->y, w->x + 1, 'o', VGA_ATTR(VGA_LIGHT_RED, active ? VGA_BLUE : VGA_DARK_GRAY));
    gfx_put_cell(w->y, w->x + 3, 'o', VGA_ATTR(VGA_YELLOW, active ? VGA_BLUE : VGA_DARK_GRAY));
    gfx_put_cell(w->y, w->x + 5, 'o', VGA_ATTR(VGA_LIGHT_GREEN, active ? VGA_BLUE : VGA_DARK_GRAY));
    gfx_write_text(w->y, w->x + 8, w->title, title_attr);

    for (int r = w->y + 1; r < w->y + w->h; r++) {
        gfx_put_cell(r, w->x, '|', frame_attr);
        gfx_fill_rect(r, w->x + 1, 1, w->w - 2, ' ', content_attr);
        gfx_put_cell(r, w->x + w->w - 1, '|', frame_attr);
    }

    for (int c = w->x; c < w->x + w->w; c++) {
        gfx_put_cell(w->y + w->h - 1, c, '-', frame_attr);
    }

    draw_window_content(w);
}

static void draw_dock(void) {
    uint8_t dock_bg = VGA_ATTR(VGA_BLACK, VGA_BLUE);
    uint8_t dock_attr = VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY);
    uint8_t icon_attr = VGA_ATTR(VGA_WHITE, VGA_DARK_GRAY);
    uint8_t icon_sel_attr = VGA_ATTR(VGA_WHITE, VGA_BLUE);

    gfx_fill_rect(DOCK_TOP, 0, DOCK_HEIGHT, VGA_WIDTH, ' ', dock_bg);
    gfx_fill_rect(DOCK_TOP, DOCK_START_X - 2, 3, WINDOW_COUNT * DOCK_SLOT_W + 4, ' ', dock_attr);

    for (int i = 0; i < WINDOW_COUNT; i++) {
        int x = dock_slot_x(i);
        uint8_t attr = (i == g_dock_index) ? icon_sel_attr : icon_attr;
        gfx_fill_rect(DOCK_TOP + 1, x, 1, DOCK_SLOT_W - 1, ' ', attr);
        gfx_write_text(DOCK_TOP + 1, x + 1, g_dock_items[i], attr);
    }
}

static int window_hit_test(int x, int y) {
    for (int i = WINDOW_COUNT - 1; i >= 0; i--) {
        int idx = g_zorder[i];
        desk_window_t *w = &g_windows[idx];
        if (!w->visible) continue;

        if (x >= w->x && x < w->x + w->w && y >= w->y && y < w->y + w->h) {
            return idx;
        }
    }
    return -1;
}

static int dock_item_hit_test(int x, int y) {
    if (y != (DOCK_TOP + 1)) return -1;

    for (int i = 0; i < WINDOW_COUNT; i++) {
        int sx = dock_slot_x(i);
        int ex = sx + DOCK_SLOT_W - 1;
        if (x >= sx && x < ex) return i;
    }
    return -1;
}

static bool menu_contains(int x, int y) {
    return x >= MENU_X && x < (MENU_X + MENU_W) && y >= MENU_Y && y < (MENU_Y + MENU_H);
}

static int menu_item_hit_test(int x, int y) {
    if (!menu_contains(x, y)) return -1;
    if (y == MENU_Y + 1) return 0;
    if (y == MENU_Y + 2) return 1;
    if (y == MENU_Y + 3) return 2;
    if (y == MENU_Y + 4) return 3;
    return -1;
}

static void reset_layout(void) {
    g_windows[0].x = 4;  g_windows[0].y = 4;  g_windows[0].visible = true;
    g_windows[1].x = 18; g_windows[1].y = 6;  g_windows[1].visible = true;
    g_windows[2].x = 28; g_windows[2].y = 5;  g_windows[2].visible = true;
    g_windows[3].x = 12; g_windows[3].y = 8;  g_windows[3].visible = false;

    g_zorder[0] = 0;
    g_zorder[1] = 1;
    g_zorder[2] = 3;
    g_zorder[3] = 2;

    g_active_win = 2;
    g_dock_index = 0;
    g_menu_open = false;
    g_drag_active = false;
    g_drag_window = -1;
    g_left_prev = false;
}

static void draw_mouse_cursor(void) {
    uint16_t cell = gfx_read_cell(g_mouse_y, g_mouse_x);
    char ch = (char)(cell & 0xFFu);
    uint8_t attr = (uint8_t)(cell >> 8);
    uint8_t fg = (uint8_t)(attr & 0x0Fu);
    uint8_t bg = (uint8_t)((attr >> 4) & 0x0Fu);

    if (ch == ' ') ch = '+';
    gfx_put_cell(g_mouse_y, g_mouse_x, ch, VGA_ATTR(bg, fg));
}

static void render_desktop(void) {
    draw_wallpaper();

    for (int i = 0; i < WINDOW_COUNT; i++) {
        int idx = g_zorder[i];
        if (idx == g_active_win) continue;
        draw_window_frame(&g_windows[idx], false);
    }
    draw_window_frame(&g_windows[g_active_win], true);

    draw_menu_bar();
    draw_dock();

    if (g_mouse_ok) {
        gfx_write_text(21, 2, "Mouse: click menu and dock, drag title bars, Esc exits", VGA_ATTR(VGA_WHITE, VGA_BLUE));
    } else {
        gfx_write_text(21, 2, "Mouse init failed. Use Tab focus + Enter dock fallback. Esc exits", VGA_ATTR(VGA_WHITE, VGA_BLUE));
    }

    draw_mouse_cursor();
}

static void handle_menu_action(int item) {
    if (item == 0) {
        int about = 2;
        g_windows[about].visible = true;
        bring_to_front(about);
    } else if (item == 1) {
        reset_layout();
    } else if (item == 2) {
        g_windows[g_active_win].visible = false;
    }
    g_menu_open = false;
}

static void handle_mouse_press(void) {
    if (g_mouse_y == 0 && g_mouse_x <= 8) {
        g_menu_open = !g_menu_open;
        return;
    }

    if (g_menu_open) {
        int item = menu_item_hit_test(g_mouse_x, g_mouse_y);
        if (item >= 0) {
            handle_menu_action(item);
            return;
        }
        if (!menu_contains(g_mouse_x, g_mouse_y)) {
            g_menu_open = false;
        }
    }

    int dock_idx = dock_item_hit_test(g_mouse_x, g_mouse_y);
    if (dock_idx >= 0) {
        int win_idx = window_from_dock(dock_idx);
        g_dock_index = dock_idx;
        g_windows[win_idx].visible = true;
        bring_to_front(win_idx);
        return;
    }

    int win_idx = window_hit_test(g_mouse_x, g_mouse_y);
    if (win_idx >= 0) {
        desk_window_t *w = &g_windows[win_idx];
        bring_to_front(win_idx);

        if (g_mouse_y == w->y && g_mouse_x >= (w->x + 1) && g_mouse_x <= (w->x + 2)) {
            w->visible = false;
            return;
        }

        if (g_mouse_y == w->y) {
            g_drag_active = true;
            g_drag_window = win_idx;
        }
    }
}

static void process_mouse(void) {
    int dx = 0;
    int dy = 0;

    mouse_poll();
    mouse_get_screen_pos(&g_mouse_x, &g_mouse_y);
    mouse_get_delta(&dx, &dy);

    bool left = (mouse_get_buttons() & MOUSE_BTN_LEFT) != 0;

    if (g_drag_active && g_drag_window >= 0 && left) {
        desk_window_t *w = &g_windows[g_drag_window];
        w->x += dx;
        w->y += dy;
        clamp_window(w);
    }

    if (left && !g_left_prev) {
        handle_mouse_press();
    }

    if (!left && g_left_prev) {
        g_drag_active = false;
        g_drag_window = -1;
    }

    g_left_prev = left;
}

static void handle_keyboard_fallback(int key) {
    if (key == KEY_TAB) {
        int pos = 0;
        for (; pos < WINDOW_COUNT; pos++) {
            if (g_zorder[pos] == g_active_win) break;
        }
        pos = (pos + 1) % WINDOW_COUNT;
        bring_to_front(g_zorder[pos]);
        g_dock_index = g_active_win;
    } else if (key == KEY_LEFT) {
        g_dock_index--;
        if (g_dock_index < 0) g_dock_index = WINDOW_COUNT - 1;
    } else if (key == KEY_RIGHT) {
        g_dock_index++;
        if (g_dock_index >= WINDOW_COUNT) g_dock_index = 0;
    } else if (key == KEY_ENTER) {
        int win_idx = window_from_dock(g_dock_index);
        g_windows[win_idx].visible = true;
        bring_to_front(win_idx);
    } else if (key == 'm' || key == 'M') {
        g_menu_open = !g_menu_open;
    }
}

void desktop_run(void) {
    gfx_init();
    g_mouse_ok = mouse_init() != 0;

    reset_layout();
    g_frame_counter = 0;
    mouse_set_screen_pos(VGA_WIDTH / 2, VGA_HEIGHT / 2);

    for (;;) {
        g_frame_counter++;

        if (g_mouse_ok) {
            process_mouse();
        }

        int key = keyboard_poll();
        if (key == KEY_ESCAPE) {
            return;
        }
        if (key) {
            handle_keyboard_fallback(key);
        }

        render_desktop();

        for (volatile uint32_t i = 0; i < 220000; i++) {
            (void)i;
        }
    }
}
