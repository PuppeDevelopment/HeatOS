#ifndef IMAGE_H
#define IMAGE_H

#include "ramdisk.h"

typedef enum {
    IMAGE_FMT_NONE = 0,
    IMAGE_FMT_PPM,
    IMAGE_FMT_BMP,
} image_format_t;

typedef struct {
    image_format_t format;
    int width;
    int height;
} image_info_t;

bool        image_get_info(fs_node_t node, image_info_t *info);
bool        image_draw_file(fs_node_t node, int x, int y, int max_w, int max_h, uint32_t bg_color);
const char *image_format_name(image_format_t format);

#endif