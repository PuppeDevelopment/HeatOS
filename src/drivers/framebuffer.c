#include "framebuffer.h"
#include "bga.h"
#include "font_8x16.h"
#include "string.h"

uint16_t fb_width = 800;
uint16_t fb_height = 600;
static volatile uint32_t *fb = 0;
/* Allocate backbuffer at 4MB mark to avoid clobbering lower memory in real/early protected mode */
static uint32_t *backbuffer = (uint32_t *)0x00400000;

bool fb_init(void) {
    uint32_t phys = bga_get_framebuffer();
    if (!phys) return false;
    fb = (volatile uint32_t *)phys;
    bga_set_video_mode(800, 600, 32, true, true);
    fb_width = 800;
    fb_height = 600;
    
    // Clear backbuffer
    memset(backbuffer, 0, fb_width * fb_height * 4);
    fb_swap(); // Clear frontbuffer too
    
    return true;
}

void fb_put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < fb_width && y >= 0 && y < fb_height) {
        backbuffer[y * fb_width + x] = color;
    }
}

void fb_fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int r = y; r < y + h; r++) {
        if (r < 0 || r >= fb_height) continue;
        for (int c = x; c < x + w; c++) {
            if (c < 0 || c >= fb_width) continue;
            backbuffer[r * fb_width + c] = color;
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

// Bresenham's Line Algorithm
void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int stepx, stepy;

    if (dy < 0) { dy = -dy; stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx; stepx = -1; } else { stepx = 1; }

    dy <<= 1;
    dx <<= 1;

    fb_put_pixel(x0, y0, color);
    if (dx > dy) {
        int fraction = dy - (dx >> 1);
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;
            }
            x0 += stepx;
            fraction += dy;
            fb_put_pixel(x0, y0, color);
        }
    } else {
        int fraction = dx - (dy >> 1);
        while (y0 != y1) {
            if (fraction >= 0) {
                x0 += stepx;
                fraction -= dy;
            }
            y0 += stepy;
            fraction += dx;
            fb_put_pixel(x0, y0, color);
        }
    }
}

void fb_draw_circle(int xc, int yc, int r, uint32_t color) {
    int x = 0, y = r;
    int d = 3 - 2 * r;
    while (y >= x) {
        fb_put_pixel(xc + x, yc + y, color);
        fb_put_pixel(xc - x, yc + y, color);
        fb_put_pixel(xc + x, yc - y, color);
        fb_put_pixel(xc - x, yc - y, color);
        fb_put_pixel(xc + y, yc + x, color);
        fb_put_pixel(xc - y, yc + x, color);
        fb_put_pixel(xc + y, yc - x, color);
        fb_put_pixel(xc - y, yc - x, color);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void fb_fill_circle(int xc, int yc, int r, uint32_t color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                fb_put_pixel(xc + x, yc + y, color);
            }
        }
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
    if (!fb) return;
    memcpy((void*)fb, backbuffer, 800 * 600 * 4);
}
