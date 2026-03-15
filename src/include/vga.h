#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_ADDRESS  0xB8000
#define VGA_WIDTH    80
#define VGA_HEIGHT   25

/* Color constants */
#define VGA_BLACK        0
#define VGA_BLUE         1
#define VGA_GREEN        2
#define VGA_CYAN         3
#define VGA_RED          4
#define VGA_MAGENTA      5
#define VGA_BROWN        6
#define VGA_LIGHT_GRAY   7
#define VGA_DARK_GRAY    8
#define VGA_LIGHT_BLUE   9
#define VGA_LIGHT_GREEN  10
#define VGA_LIGHT_CYAN   11
#define VGA_LIGHT_RED    12
#define VGA_LIGHT_MAGENTA 13
#define VGA_YELLOW       14
#define VGA_WHITE        15

#define VGA_ATTR(fg, bg)  ((uint8_t)((bg) << 4 | (fg)))

/* Popeye Plasma color scheme (KDE Plasma-like) */
#define THEME_DESKTOP    VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLUE)     /* 0x1B */
#define THEME_TOPBAR     VGA_ATTR(VGA_WHITE, VGA_DARK_GRAY)     /* 0x8F */
#define THEME_PANEL      VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY)    /* 0x70 */
#define THEME_SELECTED   VGA_ATTR(VGA_WHITE, VGA_CYAN)          /* 0x3F */
#define THEME_NORMAL     VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLUE)     /* 0x1B */
#define THEME_TITLE      VGA_ATTR(VGA_YELLOW, VGA_BLUE)         /* 0x1E */
#define THEME_MENU_BG    VGA_ATTR(VGA_WHITE, VGA_DARK_GRAY)     /* 0x8F */
#define THEME_MENU_SEL   VGA_ATTR(VGA_WHITE, VGA_CYAN)          /* 0x3F */
#define THEME_CLOSE_BTN  VGA_ATTR(VGA_WHITE, VGA_RED)           /* 0x4F */
#define THEME_TERMINAL   VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK)   /* 0x0A */
#define THEME_APP_BG     VGA_ATTR(VGA_WHITE, VGA_BLUE)          /* 0x1F */
#define THEME_BORDER     VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLUE)     /* 0x17 */

void vga_init(void);
void vga_clear(uint8_t attr);
void vga_putchar_at(int row, int col, char c, uint8_t attr);
void vga_write_at(int row, int col, const char *s, uint8_t attr);
void vga_fill_rect(int row, int col, int h, int w, char c, uint8_t attr);
void vga_fill_row(int row, char c, uint8_t attr);
uint16_t vga_read_at(int row, int col);

/* Scrolling terminal support */
void vga_scroll_up(int top, int bottom, uint8_t attr);

#endif
