#include "plinko.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "types.h"

#define GRID_ROWS 8
#define PEG_X_SPACING 4
#define PEG_Y_SPACING 2
#define START_X 40
#define START_Y 2
#define BOARD_L 15
#define BOARD_T 1

static uint32_t plinko_rng = 0x54123456u;
static int credits = 250;
static int last_win = 0;

static uint32_t rng_next() {
    plinko_rng = plinko_rng * 1664525u + 1013904223u;
    return plinko_rng;
}

static void plinko_draw_layout() {
    vga_clear(THEME_APP_BG);
    
    // Draw Border
    uint8_t border_attr = THEME_BORDER;
    vga_fill_rect(BOARD_T, BOARD_L, 22, 50, ' ', border_attr);
    vga_fill_rect(BOARD_T + 1, BOARD_L + 1, 20, 48, ' ', THEME_APP_BG);

    vga_write_at(BOARD_T, BOARD_L + 18, " PLINKO DELUXE ", THEME_TITLE);
    
    // Draw Pegs
    uint8_t peg_attr = VGA_ATTR(VGA_WHITE, VGA_BLUE);
    for (int r = 0; r < GRID_ROWS; r++) {
        int pegs = r + 1;
        int row_x_start = START_X - (r * (PEG_X_SPACING / 2));
        for (int p = 0; p < pegs; p++) {
            vga_putchar_at(START_Y + (r * PEG_Y_SPACING), row_x_start + (p * PEG_X_SPACING), '.', peg_attr);
        }
    }

    // Draw Buckets
    const char* labels[] = { "500", "200", "100", "50", "10", "50", "100", "200", "500" };
    int bucket_y = START_Y + GRID_ROWS * PEG_Y_SPACING;
    int first_bucket_x = START_X - (GRID_ROWS * (PEG_X_SPACING / 2));
    
    for (int i = 0; i < GRID_ROWS + 1; i++) {
        int bx = first_bucket_x + (i * PEG_X_SPACING) - 1;
        vga_write_at(bucket_y, bx, labels[i], VGA_ATTR(VGA_YELLOW, VGA_DARK_GRAY));
        vga_putchar_at(bucket_y - 1, bx + 1, '|', border_attr);
    }
    
    // Stats
    vga_write_at(BOARD_T + 1, BOARD_L + 52, "Credits:   ", THEME_TOPBAR);
    char sbuf[16];
    itoa(credits, sbuf, 10);
    vga_write_at(BOARD_T + 2, BOARD_L + 52, "          ", VGA_ATTR(VGA_BLACK, VGA_BLACK));
    vga_write_at(BOARD_T + 2, BOARD_L + 52, sbuf, VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK));
    
    vga_write_at(BOARD_T + 4, BOARD_L + 52, "Last Win:  ", THEME_TOPBAR);
    itoa(last_win, sbuf, 10);
    vga_write_at(BOARD_T + 5, BOARD_L + 52, "          ", VGA_ATTR(VGA_BLACK, VGA_BLACK));
    vga_write_at(BOARD_T + 5, BOARD_L + 52, sbuf, VGA_ATTR(VGA_YELLOW, VGA_BLACK));

    vga_write_at(BOARD_T + 8, BOARD_L + 52, "SPACE to Drop", THEME_TOPBAR);
    vga_write_at(BOARD_T + 9, BOARD_L + 52, "-10 Credits", VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK));
    vga_write_at(BOARD_T + 11, BOARD_L + 52, "ESC to Exit ", THEME_TOPBAR);
}

static void animation_delay(int ms) {
    for (int d = 0; d < ms; d++) {
        for (volatile uint32_t i = 0; i < 1000000; i++) {
            // busy wait
        }
    }
}

void plinko_run() {
    for (;;) {
        plinko_draw_layout();
        
        int k;
        while ((k = keyboard_poll()) == 0) {
            plinko_rng = plinko_rng * 1664525u + 1013904223u; // Spin RNG
        }
        
        if (k == KEY_ESCAPE) return;
        if (k == ' ' || k == KEY_ENTER) {
            if (credits < 10) continue;
            credits -= 10;
            
            int cur_x = START_X;
            int cur_y = START_Y - 1;
            int row = -1;
            
            plinko_draw_layout();
            
            // Initial draw
            vga_putchar_at(cur_y, cur_x, 'O', VGA_ATTR(VGA_LIGHT_RED, VGA_BLUE));
            animation_delay(15);

            while (row < GRID_ROWS - 1) {
                // Erase old ball
                vga_putchar_at(cur_y, cur_x, ' ', THEME_APP_BG);
                
                // Move down
                row++;
                cur_y += PEG_Y_SPACING;
                
                // Decide left or right - use higher bits of RNG to avoid periodicity in lower bits
                if ((rng_next() >> 16) % 2 == 0) {
                    cur_x -= (PEG_X_SPACING / 2);
                } else {
                    cur_x += (PEG_X_SPACING / 2);
                }
                
                // Draw new ball
                vga_putchar_at(cur_y, cur_x, 'O', VGA_ATTR(VGA_LIGHT_RED, VGA_BLUE));
                animation_delay(25);
            }
            
            // Calculate win
            int first_bucket_x = START_X - (GRID_ROWS * (PEG_X_SPACING / 2));
            int bucket_idx = (cur_x - first_bucket_x) / PEG_X_SPACING;
            if (bucket_idx < 0) bucket_idx = 0;
            if (bucket_idx >= GRID_ROWS + 1) bucket_idx = GRID_ROWS;
            
            int multipliers[] = { 500, 200, 100, 50, 10, 50, 100, 200, 500 };
            last_win = multipliers[bucket_idx];
            credits += last_win;
            
            // Highlight win bucket
            vga_write_at(START_Y + GRID_ROWS * PEG_Y_SPACING, first_bucket_x + (bucket_idx * PEG_X_SPACING) - 1, "WIN!", VGA_ATTR(VGA_WHITE, VGA_RED));
            
            // Drain keyboard buffer
            while (keyboard_poll() != 0);
            
            animation_delay(80);
        }
    }
}
