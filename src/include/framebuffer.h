#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "types.h"

extern uint16_t fb_width;
extern uint16_t fb_height;

bool fb_init(void);
void fb_put_pixel(int x, int y, uint32_t color);
void fb_fill_rect(int x, int y, int w, int h, uint32_t color);
void fb_draw_rect(int x, int y, int w, int h, uint32_t color);
void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void fb_draw_circle(int xc, int yc, int r, uint32_t color);
void fb_fill_circle(int xc, int yc, int r, uint32_t color);
void fb_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void fb_draw_string(int x, int y, const char *s, uint32_t fg, uint32_t bg);

/* Optional double buffering swap */
void fb_swap(void);

#endif
