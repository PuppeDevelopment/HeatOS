/*=============================================================================
 * Popeye Plasma Desktop Environment — KDE Plasma-like text-mode desktop
 * Main event loop, rendering, kickoff menu, run dialog
 *===========================================================================*/
#include "desktop.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "io.h"

/* Forward declarations for apps (in apps.c) */
void app_terminal(void);
void app_system(void);
void app_files(void);
void app_notes(void);
void app_clock(void);
void app_network(void);
void app_power(void);

/* ---- RTC helpers (CMOS ports 0x70/0x71) ---- */
static uint8_t rtc_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}



/* ---- Desktop state ---- */
static int selection = 0;

static const char *app_names[APP_COUNT] = {
    "  Terminal       ",
    "  System Info    ",
    "  Files          ",
    "  Notes          ",
    "  Clock          ",
    "  Network        ",
    "  Power          ",
};

static const char *app_icons[APP_COUNT] = {
    ">_", "SI", "FI", "NO", "CL", "NE", "PW",
};

typedef void (*app_func_t)(void);
static const app_func_t app_handlers[APP_COUNT] = {
    app_terminal, app_system, app_files, app_notes,
    app_clock, app_network, app_power,
};

/* ---- Clock string for panel ---- */
static void get_time_str(char *buf) {
    uint8_t h = bcd_to_bin(rtc_read(0x04));
    uint8_t m = bcd_to_bin(rtc_read(0x02));
    buf[0] = '0' + h / 10; buf[1] = '0' + h % 10;
    buf[2] = ':';
    buf[3] = '0' + m / 10; buf[4] = '0' + m % 10;
    buf[5] = '\0';
}

/* ---- Render the desktop home screen ---- */
static void render_desktop(void) {
    /* Clear */
    vga_clear(THEME_DESKTOP);

    /* Top bar (row 0) */
    vga_fill_row(0, ' ', THEME_TOPBAR);
    vga_write_at(0, 1, "Popeye Plasma", THEME_TOPBAR);

    /* Clock in top bar */
    char timebuf[8];
    get_time_str(timebuf);
    vga_write_at(0, VGA_WIDTH - 7, timebuf, THEME_TOPBAR);

    /* Desktop title */
    vga_write_at(2, 30, ":: RushOS Desktop ::", THEME_TITLE);

    /* App list (centered) */
    for (int i = 0; i < APP_COUNT; i++) {
        int row = 5 + i * 2;
        uint8_t attr = (i == selection) ? THEME_SELECTED : THEME_NORMAL;
        uint8_t icon_attr = (i == selection) ? THEME_SELECTED : THEME_TITLE;

        /* Icon */
        vga_write_at(row, 25, app_icons[i], icon_attr);
        /* Name */
        vga_write_at(row, 28, app_names[i], attr);

        /* Selection indicator */
        if (i == selection)
            vga_putchar_at(row, 23, '>', THEME_TITLE);
    }

    /* Bottom panel (row 24) */
    vga_fill_row(24, ' ', THEME_PANEL);
    vga_write_at(24, 1, "[M] Kickoff", THEME_PANEL);
    vga_write_at(24, 15, "[F2] Terminal", THEME_PANEL);
    vga_write_at(24, 31, "[R] Run", THEME_PANEL);

    /* Hints */
    vga_write_at(22, 20, "Up/Down: Navigate   Enter: Open", VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLUE));
}

/* ---- Kickoff (start) menu ---- */
static void kickoff_menu(void) {
    int sel = 0;
    for (;;) {
        /* Draw overlay */
        vga_fill_rect(10, 20, APP_COUNT + 4, 30, ' ', THEME_MENU_BG);

        /* Border */
        for (int c = 20; c < 50; c++) {
            vga_putchar_at(10, c, 0xCD, THEME_MENU_BG);
            vga_putchar_at(10 + APP_COUNT + 3, c, 0xCD, THEME_MENU_BG);
        }
        vga_write_at(11, 22, "Kickoff Menu", VGA_ATTR(VGA_YELLOW, VGA_DARK_GRAY));

        for (int i = 0; i < APP_COUNT; i++) {
            int row = 12 + i;
            uint8_t attr = (i == sel) ? THEME_MENU_SEL : THEME_MENU_BG;
            vga_write_at(row, 22, app_names[i], attr);
            if (i == sel)
                vga_putchar_at(row, 21, '>', VGA_ATTR(VGA_YELLOW, VGA_DARK_GRAY));
            else
                vga_putchar_at(row, 21, ' ', THEME_MENU_BG);
        }

        int key = keyboard_wait();
        if (key == KEY_ESCAPE || key == 'm' || key == 'M') return;
        if (key == KEY_UP && sel > 0) sel--;
        if (key == KEY_DOWN && sel < APP_COUNT - 1) sel++;
        if (key == KEY_ENTER) {
            app_handlers[sel]();
            return;
        }
        /* Number keys 1-7 */
        if (key >= '1' && key <= '0' + APP_COUNT) {
            app_handlers[key - '1']();
            return;
        }
    }
}

/* ---- Run dialog ---- */
static void run_dialog(void) {
    char cmd[32];
    int pos = 0;

    vga_fill_rect(11, 20, 3, 40, ' ', THEME_MENU_BG);
    vga_write_at(11, 22, "Run:", VGA_ATTR(VGA_YELLOW, VGA_DARK_GRAY));
    vga_write_at(12, 22, "> ", THEME_MENU_BG);

    for (;;) {
        vga_putchar_at(12, 24 + pos, '_', THEME_MENU_BG);  /* cursor */

        int key = keyboard_wait();
        if (key == KEY_ESCAPE) return;
        if (key == KEY_BACKSPACE && pos > 0) {
            pos--;
            vga_putchar_at(12, 24 + pos, ' ', THEME_MENU_BG);
            vga_putchar_at(12, 24 + pos + 1, ' ', THEME_MENU_BG);
            cmd[pos] = '\0';
            continue;
        }
        if (key == KEY_ENTER) {
            cmd[pos] = '\0';
            /* Match command to app */
            if (strcmp(cmd, "term") == 0 || strcmp(cmd, "terminal") == 0) { app_terminal(); return; }
            if (strcmp(cmd, "system") == 0 || strcmp(cmd, "sysinfo") == 0) { app_system(); return; }
            if (strcmp(cmd, "files") == 0) { app_files(); return; }
            if (strcmp(cmd, "notes") == 0) { app_notes(); return; }
            if (strcmp(cmd, "clock") == 0 || strcmp(cmd, "time") == 0)  { app_clock(); return; }
            if (strcmp(cmd, "network") == 0 || strcmp(cmd, "net") == 0) { app_network(); return; }
            if (strcmp(cmd, "power") == 0 || strcmp(cmd, "reboot") == 0) { app_power(); return; }
            /* Unknown - flash */
            vga_write_at(13, 22, "Unknown app!", VGA_ATTR(VGA_LIGHT_RED, VGA_DARK_GRAY));
            keyboard_wait();
            return;
        }
        /* Printable character */
        if (key >= 32 && key < 127 && pos < 28) {
            cmd[pos] = (char)key;
            vga_putchar_at(12, 24 + pos, (char)key, THEME_MENU_BG);
            pos++;
        }
    }
}

/* ---- App window frame ---- */
void draw_app_frame(const char *title) {
    vga_clear(THEME_APP_BG);

    /* Title bar */
    vga_fill_row(0, ' ', THEME_TOPBAR);
    vga_write_at(0, 2, title, THEME_TOPBAR);
    vga_write_at(0, VGA_WIDTH - 5, "[Esc]", THEME_CLOSE_BTN);

    /* Border lines */
    vga_fill_row(1, '-', THEME_BORDER);

    /* Status bar */
    vga_fill_row(24, ' ', THEME_PANEL);
    vga_write_at(24, 1, "Esc: Back to Desktop", THEME_PANEL);
}

/* ---- Main desktop loop ---- */
void desktop_run(void) {
    for (;;) {
        render_desktop();

        int key = keyboard_wait();

        switch (key) {
            case KEY_UP:
                if (selection > 0) selection--;
                break;
            case KEY_DOWN:
                if (selection < APP_COUNT - 1) selection++;
                break;
            case KEY_ENTER:
                app_handlers[selection]();
                break;
            case KEY_F2:
            case 't': case 'T':
                app_terminal();
                break;
            case 'm': case 'M':
                kickoff_menu();
                break;
            case 'r': case 'R':
                run_dialog();
                break;
            case 's': case 'S':
                app_system();
                break;
            case 'f': case 'F':
                app_files();
                break;
            case 'n': case 'N':
                app_notes();
                break;
            case 'c': case 'C':
                app_clock();
                break;
            case 'w': case 'W':
                app_network();
                break;
            case 'p': case 'P':
                app_power();
                break;
            default:
                /* Number keys 1-7 */
                if (key >= '1' && key <= '0' + APP_COUNT) {
                    selection = key - '1';
                    app_handlers[selection]();
                }
                break;
        }
    }
}
