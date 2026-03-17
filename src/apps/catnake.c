#include "catnake.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "types.h"

#define CATNAKE_WIDTH 40
#define CATNAKE_HEIGHT 18
#define CATNAKE_X 20
#define CATNAKE_Y 3
#define CATNAKE_MAX (CATNAKE_WIDTH * CATNAKE_HEIGHT)

typedef struct { int x; int y; } pt_t;

static uint32_t catnake_rng = 0xC0FFEE11u;
static int pending_input = 0;
static int high_score = 0;

static uint32_t rng_next() {
    catnake_rng = catnake_rng * 1664525u + 1013904223u;
    return catnake_rng;
}

static void catnake_draw_border() {
    uint8_t border_attr = VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK);
    uint8_t title_attr = VGA_ATTR(VGA_YELLOW, VGA_BLACK);
    
    // Horizontal borders
    for (int x = 0; x < CATNAKE_WIDTH + 2; x++) {
        vga_putchar_at(CATNAKE_Y - 1, CATNAKE_X - 1 + x, '=', border_attr);
        vga_putchar_at(CATNAKE_Y + CATNAKE_HEIGHT, CATNAKE_X - 1 + x, '=', border_attr);
    }
    // Vertical borders
    for (int y = 0; y < CATNAKE_HEIGHT; y++) {
        vga_putchar_at(CATNAKE_Y + y, CATNAKE_X - 1, '|', border_attr);
        vga_putchar_at(CATNAKE_Y + y, CATNAKE_X + CATNAKE_WIDTH, '|', border_attr);
    }
    
    // Corners
    vga_putchar_at(CATNAKE_Y - 1, CATNAKE_X - 1, '+', border_attr);
    vga_putchar_at(CATNAKE_Y - 1, CATNAKE_X + CATNAKE_WIDTH, '+', border_attr);
    vga_putchar_at(CATNAKE_Y + CATNAKE_HEIGHT, CATNAKE_X - 1, '+', border_attr);
    vga_putchar_at(CATNAKE_Y + CATNAKE_HEIGHT, CATNAKE_X + CATNAKE_WIDTH, '+', border_attr);

    vga_write_at(CATNAKE_Y - 1, CATNAKE_X + (CATNAKE_WIDTH / 2) - 4, " CATNAKE ", title_attr);
}

static void catnake_draw_cell(int x, int y, char ch, uint8_t attr) {
    vga_putchar_at(CATNAKE_Y + y, CATNAKE_X + x, ch, attr);
}

static bool catnake_occupied(const pt_t *body, int len, int x, int y) {
    for (int i = 0; i < len; i++)
        if (body[i].x == x && body[i].y == y) return true;
    return false;
}

static pt_t catnake_place_food(const pt_t *body, int len) {
    pt_t f;
    for (int i = 0; i < 1000; i++) {
        f.x = rng_next() % CATNAKE_WIDTH;
        f.y = rng_next() % CATNAKE_HEIGHT;
        if (!catnake_occupied(body, len, f.x, f.y)) return f;
    }
    // Fallback if full (highly unlikely)
    f.x = -1; f.y = -1;
    return f;
}

static void catnake_render_all(const pt_t *body, int len, pt_t food, int score, bool dead, pt_t old_tail) {
    uint8_t bg = VGA_ATTR(VGA_WHITE, VGA_BLACK);
    
    // Optimize: only clear the old tail segment instead of the whole screen
    if (old_tail.x != -1) {
        catnake_draw_cell(old_tail.x, old_tail.y, ' ', bg);
    }
            
    // catnake_draw_border(); // Borders drawn once initially below
    
    vga_write_at(1, 2, "Catnake Deluxe  (WASD/Arrows)  Esc=Quit", VGA_ATTR(VGA_WHITE, VGA_BLACK));
    vga_write_at(1, 45, "Score:", VGA_ATTR(VGA_WHITE, VGA_BLACK));
    char sbuf[12];
    itoa(score, sbuf, 10);
    vga_write_at(1, 52, sbuf, VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK));
    
    vga_write_at(1, 60, "Best:", VGA_ATTR(VGA_WHITE, VGA_BLACK));
    itoa(high_score, sbuf, 10);
    vga_write_at(1, 66, sbuf, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
    
    // Draw food - mouse emoji-like char or asterisk?
    catnake_draw_cell(food.x, food.y, '*', VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK));
    
    // Draw body
    for (int i = len - 1; i >= 0; i--) {
        char ch;
        uint8_t attr;
        if (i == 0) { 
            ch = 'C'; 
            attr = dead ? VGA_ATTR(VGA_DARK_GRAY, VGA_BLACK) : VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK); 
        } else if (i == 1) { 
            ch = '~'; 
            attr = dead ? VGA_ATTR(VGA_DARK_GRAY, VGA_BLACK) : VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK); 
        } else { 
            ch = 'o'; 
            attr = dead ? VGA_ATTR(VGA_DARK_GRAY, VGA_BLACK) : VGA_ATTR(VGA_GREEN, VGA_BLACK); 
        }
        catnake_draw_cell(body[i].x, body[i].y, ch, attr);
    }

    if (dead) {
        vga_write_at(CATNAKE_Y + CATNAKE_HEIGHT / 2 - 1, CATNAKE_X + CATNAKE_WIDTH / 2 - 9, "  GAME OVER!  ", VGA_ATTR(VGA_WHITE, VGA_RED));
        vga_write_at(CATNAKE_Y + CATNAKE_HEIGHT / 2 + 1, CATNAKE_X + CATNAKE_WIDTH / 2 - 14, " Press ENTER to Restart ", VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK));
    }
}

void catnake_run() {
    for (;;) {
        catnake_rng = 0xC0FFEE11u + (uint32_t)keyboard_poll(); // More randomness
        pt_t body[CATNAKE_MAX];
        int len = 4;
        int dirx = 1, diry = 0;
        int pending_dirx = 1, pending_diry = 0;
        int score = 0;
        int sx = CATNAKE_WIDTH / 4, sy = CATNAKE_HEIGHT / 2;
        
        for (int i = 0; i < len; i++) { 
            body[i].x = sx - i; 
            body[i].y = sy; 
        }
        
        pt_t food = catnake_place_food(body, len);
        vga_clear(VGA_ATTR(VGA_WHITE, VGA_BLACK));
        catnake_draw_border();
        bool dead = false;
        pending_input = 0;
        pt_t old_tail = {-1, -1};

        for (;;) {
            int k;
            while ((k = keyboard_poll()) != 0) pending_input = k;
            
            if (pending_input == KEY_ESCAPE) return;

            if (!dead) {
                if (pending_input) {
                    if (pending_input == KEY_UP || pending_input == 'w' || pending_input == 'W') { 
                        if (diry == 0) { pending_dirx = 0; pending_diry = -1; }
                    }
                    if (pending_input == KEY_DOWN || pending_input == 's' || pending_input == 'S') { 
                        if (diry == 0) { pending_dirx = 0; pending_diry = 1; }
                    }
                    if (pending_input == KEY_LEFT || pending_input == 'a' || pending_input == 'A') { 
                        if (dirx == 0) { pending_dirx = -1; pending_diry = 0; }
                    }
                    if (pending_input == KEY_RIGHT || pending_input == 'd' || pending_input == 'D') { 
                        if (dirx == 0) { pending_dirx = 1; pending_diry = 0; }
                    }
                    pending_input = 0;
                }
                
                dirx = pending_dirx; diry = pending_diry;
                
                pt_t next = { body[0].x + dirx, body[0].y + diry };
                if (next.x < 0 || next.x >= CATNAKE_WIDTH || next.y < 0 || next.y >= CATNAKE_HEIGHT) {
                    dead = true;
                } else if (catnake_occupied(body, len, next.x, next.y)) {
                    dead = true;
                } else {
                    old_tail = body[len - 1]; // remember tail to erase
                    for (int i = len - 1; i > 0; i--) body[i] = body[i - 1];
                    body[0] = next;
                    if (body[0].x == food.x && body[0].y == food.y) {
                        if (len < CATNAKE_MAX) { body[len] = old_tail; len++; }
                        old_tail.x = -1; // didn't lose tail segment, don't erase
                        score++;
                        if (score > high_score) high_score = score;
                        food = catnake_place_food(body, len);
                    }
                }
            } else {
                if (pending_input == KEY_ENTER) break;
            }
            
            catnake_render_all(body, len, food, score, dead, old_tail);
            old_tail.x = -1; // reset for next frame if dead/waiting
            
            uint32_t delay_loops;
            if (dead) delay_loops = 50;
            else {
                // Progressive speed: start at 60, getting slightly faster linearly
                delay_loops = 60 - (score / 2); // gets faster every 2 apples
                if (delay_loops < 15) delay_loops = 15; // cap the maximum speed
            }

            for (uint32_t d = 0; d < delay_loops; d++) {
                for (volatile uint32_t i = 0; i < 800000; i++) {
                    // Small busy wait
                }
                while ((k = keyboard_poll()) != 0) pending_input = k;
                if (pending_input == KEY_ESCAPE) return;
                if (dead && (pending_input == KEY_ENTER)) break;
            }
            if (dead && (pending_input == KEY_ENTER)) break;
        }
    }
}
