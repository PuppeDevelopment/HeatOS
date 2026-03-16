#include "image.h"
#include "framebuffer.h"
#include "string.h"

typedef struct {
    image_format_t format;
    int width;
    int height;
    int data_offset;
    int row_stride;
    int bits_per_pixel;
    bool top_down;
} image_parse_t;

static uint8_t g_image_file_buf[RAMDISK_DATA_CAP];

static uint16_t read_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static bool is_space(uint8_t c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void skip_ppm_ws_and_comments(const uint8_t *buf, int len, int *pos) {
    while (*pos < len) {
        if (buf[*pos] == '#') {
            while (*pos < len && buf[*pos] != '\n') {
                (*pos)++;
            }
            continue;
        }

        if (!is_space(buf[*pos])) {
            break;
        }
        (*pos)++;
    }
}

static bool parse_ppm_int(const uint8_t *buf, int len, int *pos, int *value_out) {
    int value = 0;
    bool saw_digit = false;

    skip_ppm_ws_and_comments(buf, len, pos);
    while (*pos < len) {
        uint8_t c = buf[*pos];
        if (c < '0' || c > '9') {
            break;
        }
        saw_digit = true;
        value = value * 10 + (int)(c - '0');
        (*pos)++;
    }

    if (!saw_digit) {
        return false;
    }

    *value_out = value;
    return true;
}

static bool parse_ppm(const uint8_t *buf, int len, image_parse_t *out) {
    int pos = 0;
    int width = 0;
    int height = 0;
    int maxval = 0;

    if (len < 11 || buf[0] != 'P' || buf[1] != '6') {
        return false;
    }

    pos = 2;
    if (!parse_ppm_int(buf, len, &pos, &width) ||
        !parse_ppm_int(buf, len, &pos, &height) ||
        !parse_ppm_int(buf, len, &pos, &maxval)) {
        return false;
    }

    if (width <= 0 || height <= 0 || maxval != 255) {
        return false;
    }

    while (pos < len && is_space(buf[pos])) {
        pos++;
    }

    if (pos + width * height * 3 > len) {
        return false;
    }

    out->format = IMAGE_FMT_PPM;
    out->width = width;
    out->height = height;
    out->data_offset = pos;
    out->row_stride = width * 3;
    out->bits_per_pixel = 24;
    out->top_down = true;
    return true;
}

static bool parse_bmp(const uint8_t *buf, int len, image_parse_t *out) {
    if (len < 54 || buf[0] != 'B' || buf[1] != 'M') {
        return false;
    }

    uint32_t pixel_offset = read_u32_le(&buf[10]);
    uint32_t dib_size = read_u32_le(&buf[14]);
    int32_t width = (int32_t)read_u32_le(&buf[18]);
    int32_t height = (int32_t)read_u32_le(&buf[22]);
    uint16_t planes = read_u16_le(&buf[26]);
    uint16_t bpp = read_u16_le(&buf[28]);
    uint32_t compression = read_u32_le(&buf[30]);

    if (dib_size < 40 || planes != 1 || (bpp != 24 && bpp != 32) || compression != 0) {
        return false;
    }

    if (width <= 0 || height == 0) {
        return false;
    }

    bool top_down = false;
    if (height < 0) {
        top_down = true;
        height = -height;
    }

    int row_stride = (int)((((uint32_t)width * (uint32_t)bpp) + 31u) / 32u) * 4;
    if ((int)pixel_offset + row_stride * height > len) {
        return false;
    }

    out->format = IMAGE_FMT_BMP;
    out->width = width;
    out->height = height;
    out->data_offset = (int)pixel_offset;
    out->row_stride = row_stride;
    out->bits_per_pixel = bpp;
    out->top_down = top_down;
    return true;
}

static bool parse_image_bytes(const uint8_t *buf, int len, image_parse_t *out) {
    memset(out, 0, sizeof(*out));

    if (parse_ppm(buf, len, out)) {
        return true;
    }
    if (parse_bmp(buf, len, out)) {
        return true;
    }
    return false;
}

static bool load_image(fs_node_t node, image_parse_t *parsed) {
    int bytes_read;

    if (!fs_is_file(node)) {
        return false;
    }

    bytes_read = fs_read(node, g_image_file_buf, (int)sizeof(g_image_file_buf));
    if (bytes_read <= 0) {
        return false;
    }

    return parse_image_bytes(g_image_file_buf, bytes_read, parsed);
}

static uint32_t read_image_pixel(const image_parse_t *img, int sx, int sy) {
    if (img->format == IMAGE_FMT_PPM) {
        int off = img->data_offset + sy * img->row_stride + sx * 3;
        uint8_t r = g_image_file_buf[off + 0];
        uint8_t g = g_image_file_buf[off + 1];
        uint8_t b = g_image_file_buf[off + 2];
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    if (img->format == IMAGE_FMT_BMP) {
        int source_y = img->top_down ? sy : (img->height - 1 - sy);
        int off = img->data_offset + source_y * img->row_stride + sx * (img->bits_per_pixel / 8);
        uint8_t b = g_image_file_buf[off + 0];
        uint8_t g = g_image_file_buf[off + 1];
        uint8_t r = g_image_file_buf[off + 2];
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    return 0;
}

bool image_get_info(fs_node_t node, image_info_t *info) {
    image_parse_t parsed;

    if (!info) {
        return false;
    }

    if (!load_image(node, &parsed)) {
        memset(info, 0, sizeof(*info));
        return false;
    }

    info->format = parsed.format;
    info->width = parsed.width;
    info->height = parsed.height;
    return true;
}

bool image_draw_file(fs_node_t node, int x, int y, int max_w, int max_h, uint32_t bg_color) {
    image_parse_t parsed;
    int draw_w;
    int draw_h;
    int draw_x;
    int draw_y;

    if (max_w <= 0 || max_h <= 0) {
        return false;
    }

    if (!load_image(node, &parsed)) {
        return false;
    }

    fb_fill_rect(x, y, max_w, max_h, bg_color);

    draw_w = max_w;
    draw_h = (parsed.height * max_w) / parsed.width;
    if (draw_h > max_h) {
        draw_h = max_h;
        draw_w = (parsed.width * max_h) / parsed.height;
    }

    if (draw_w < 1) draw_w = 1;
    if (draw_h < 1) draw_h = 1;

    draw_x = x + (max_w - draw_w) / 2;
    draw_y = y + (max_h - draw_h) / 2;

    for (int dy = 0; dy < draw_h; dy++) {
        int sy = (dy * parsed.height) / draw_h;
        for (int dx = 0; dx < draw_w; dx++) {
            int sx = (dx * parsed.width) / draw_w;
            fb_put_pixel(draw_x + dx, draw_y + dy, read_image_pixel(&parsed, sx, sy));
        }
    }

    return true;
}

const char *image_format_name(image_format_t format) {
    if (format == IMAGE_FMT_PPM) return "PPM";
    if (format == IMAGE_FMT_BMP) return "BMP";
    return "unknown";
}