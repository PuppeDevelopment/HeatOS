#ifndef BGA_H
#define BGA_H

#include "types.h"

uint32_t bga_get_framebuffer(void);
void bga_set_video_mode(uint16_t width, uint16_t height, uint16_t bpp, bool linear, bool clear);

#endif
