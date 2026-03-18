#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"

typedef enum {
    GFX_BACKEND_VGA_TEXT = 0,
    GFX_BACKEND_VIRTIO_TEXT = 1
} gfx_backend_t;

void gfx_init(void);
gfx_backend_t gfx_backend(void);
const char *gfx_backend_name(void);
bool gfx_virtio_present(void);

void gfx_clear(uint8_t attr);
void gfx_put_cell(int row, int col, char c, uint8_t attr);
void gfx_write_text(int row, int col, const char *s, uint8_t attr);
void gfx_fill_rect(int row, int col, int h, int w, char c, uint8_t attr);
void gfx_fill_row(int row, char c, uint8_t attr);
uint16_t gfx_read_cell(int row, int col);

#endif
