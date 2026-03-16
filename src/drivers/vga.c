#include "vga.h"
#include "string.h"

static volatile uint16_t *const VGA = (volatile uint16_t *)VGA_ADDRESS;

void vga_init(void) {
    vga_clear(VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK));
}

void vga_clear(uint8_t attr) {
    uint16_t entry = (uint16_t)' ' | ((uint16_t)attr << 8);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA[i] = entry;
}

void vga_putchar_at(int row, int col, char c, uint8_t attr) {
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH)
        VGA[row * VGA_WIDTH + col] = (uint16_t)(uint8_t)c | ((uint16_t)attr << 8);
}

void vga_write_at(int row, int col, const char *s, uint8_t attr) {
    while (*s && col < VGA_WIDTH) {
        vga_putchar_at(row, col, *s, attr);
        s++;
        col++;
    }
}

void vga_fill_rect(int row, int col, int h, int w, char c, uint8_t attr) {
    uint16_t entry = (uint16_t)(uint8_t)c | ((uint16_t)attr << 8);
    for (int r = row; r < row + h && r < VGA_HEIGHT; r++)
        for (int cc = col; cc < col + w && cc < VGA_WIDTH; cc++)
            VGA[r * VGA_WIDTH + cc] = entry;
}

void vga_fill_row(int row, char c, uint8_t attr) {
    vga_fill_rect(row, 0, 1, VGA_WIDTH, c, attr);
}

uint16_t vga_read_at(int row, int col) {
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH)
        return VGA[row * VGA_WIDTH + col];
    return 0;
}

void vga_scroll_up(int top, int bottom, uint8_t attr) {
    for (int r = top; r < bottom - 1; r++)
        memcpy((void *)&VGA[r * VGA_WIDTH], (const void *)&VGA[(r + 1) * VGA_WIDTH],
               VGA_WIDTH * sizeof(uint16_t));
    uint16_t blank = (uint16_t)' ' | ((uint16_t)attr << 8);
    for (int c = 0; c < VGA_WIDTH; c++)
        VGA[(bottom - 1) * VGA_WIDTH + c] = blank;
}
