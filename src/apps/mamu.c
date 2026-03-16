#include "mamu.h"
#include "vga.h"
#include "keyboard.h"
#include "ramdisk.h"
#include "string.h"
#include "types.h"

#define BUFFER_MAX 4096
static char text_buffer[BUFFER_MAX];
static int  text_len = 0;
static char curr_file[16];

static void draw_ui(void) {
    /* Title bar */
    vga_fill_row(0, ' ', THEME_TOPBAR);
    vga_write_at(0, 2, "Mamu Editor v0.1 - File: ", THEME_TOPBAR);
    vga_write_at(0, 27, curr_file[0] ? curr_file : "(untitled)", THEME_TOPBAR);

    /* Main area */
    vga_fill_rect(1, 0, VGA_HEIGHT - 2, VGA_WIDTH, ' ', THEME_APP_BG);

    /* Render text */
    int r = 1;
    int c = 0;
    for (int i = 0; i < text_len; i++) {
        if (r < VGA_HEIGHT - 1) {
            if (text_buffer[i] == '\n') {
                r++;
                c = 0;
            } else {
                if (c < VGA_WIDTH) {
                    vga_putchar_at(r, c, text_buffer[i], THEME_APP_BG);
                    c++;
                }
            }
        }
    }

    /* Status bar / Help */
    vga_fill_row(VGA_HEIGHT - 1, ' ', THEME_MENU_BG);
    vga_write_at(VGA_HEIGHT - 1, 2, "F2: Save    F10: Exit    Arrows: Move", THEME_MENU_BG);
}

static void save_file(void) {
    if (!curr_file[0]) return;
    
    fs_node_t node = fs_resolve(curr_file);
    if (!node) {
        /* Try to create it in current directory */
        if (!fs_create_file(fs_cwd_get(), curr_file)) {
            vga_write_at(VGA_HEIGHT - 1, 40, "Error creating file!", VGA_ATTR(VGA_WHITE, VGA_RED));
            return;
        }
        node = fs_resolve(curr_file);
    }
    
    if (node && fs_is_file(node)) {
        if (fs_write(node, text_buffer, text_len)) {
            vga_write_at(VGA_HEIGHT - 1, 40, "Saved successfully!", VGA_ATTR(VGA_WHITE, VGA_GREEN));
        } else {
            vga_write_at(VGA_HEIGHT - 1, 40, "Error writing file!", VGA_ATTR(VGA_WHITE, VGA_RED));
        }
    }
}

void mamu_run(const char *filename) {
    memset(text_buffer, 0, BUFFER_MAX);
    text_len = 0;
    memset(curr_file, 0, sizeof(curr_file));

    if (filename && filename[0]) {
        strncpy(curr_file, filename, sizeof(curr_file) - 1);
        fs_node_t node = fs_resolve(filename);
        if (node && fs_is_file(node)) {
            text_len = fs_read(node, text_buffer, BUFFER_MAX - 1);
            if (text_len < 0) text_len = 0;
            text_buffer[text_len] = '\0';
        }
    }

    vga_clear(THEME_APP_BG);
    
    for (;;) {
        draw_ui();
        /* Draw cursor - since we don't have a hardware cursor helper in vga.h that matches THEME_APP_BG 
           let's just use an invert or a character. */
        /* For now, just poll keyboard. */

        int k = keyboard_wait();
        
        if (k == KEY_F10) break;
        if (k == KEY_F2) {
            save_file();
            /* Busy wait a bit so user sees the "Saved" message */
            for (volatile int i = 0; i < 5000000; i++);
            continue;
        }

        if (k == KEY_BACKSPACE) {
            if (text_len > 0) {
                text_len--;
                text_buffer[text_len] = '\0';
            }
        } else if (k == KEY_ENTER) {
            if (text_len < BUFFER_MAX - 1) {
                text_buffer[text_len++] = '\n';
                text_buffer[text_len] = '\0';
            }
        } else if (k >= 0x20 && k < 0x7F) {
            if (text_len < BUFFER_MAX - 1) {
                text_buffer[text_len++] = (char)k;
                text_buffer[text_len] = '\0';
            }
        }
    }
}
