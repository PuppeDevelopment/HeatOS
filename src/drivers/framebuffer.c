#include "framebuffer.h"
#include "bga.h"
#include "font_8x16.h"

uint16_t fb_width = 800;
uint16_t fb_height = 600;
static volatile uint32_t *fb = 0;

bool fb_init(void) {
    uint32_t phys = bga_get_framebuffer();
    if (!phys) return false;
    fb = (volatile uint32_t *)phys;
    bga_set_video_mode(800, 600, 32, true, true);
    fb_width = 800;
    fb_height = 600;
    return true;
}

void fb_put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < fb_width && y >= 0 && y < fb_height) {
        fb[y * fb_width + x] = color;
    }
}

void fb_fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int r = y; r < y + h; r++) {
        if (r < 0 || r >= fb_height) continue;
        for (int c = x; c < x + w; c++) {
            if (c < 0 || c >= fb_width) continue;
            fb[r * fb_width + c] = color;
        }
    }
}

void fb_draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int c = x; c < x + w; c++) {
        fb_put_pixel(c, y, color);
        fb_put_pixel(c, y + h - 1, color);
    }
    for (int r = y; r < y + h; r++) {
        fb_put_pixel(x, r, color);
        fb_put_pixel(x + w - 1, r, color);
    }
}

void fb_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg) {
    const unsigned char *glyph = &font_8x16[(unsigned char)c * 16];
    for (int r = 0; r < 16; r++) {
        unsigned char row = glyph[r];
        for (int col = 0; col < 8; col++) {
            if (row & (1 << (7 - col))) {
                fb_put_pixel(x + col, y + r, fg);
            } else if (bg != 0xFF000000) { // Using 0xFF000000 as transparent flag
                fb_put_pixel(x + col, y + r, bg);
            }
        }
    }
}

void fb_draw_string(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    int cx = x;
    while (*s) {
        fb_draw_char(cx, y, *s, fg, bg);
        cx += 8;
        s++;
    }
}

void fb_swap(void) {
    // Basic driver, direct writes to screen for now.
}
