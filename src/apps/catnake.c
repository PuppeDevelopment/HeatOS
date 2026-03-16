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

static uint32_t rng_next() {
    catnake_rng = catnake_rng * 1664525u + 1013904223u;
    return catnake_rng;
}

static void catnake_draw_border() {
    uint8_t border = VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK);
    for (int x = 0; x < CATNAKE_WIDTH + 2; x++) {
        vga_putchar_at(CATNAKE_Y - 1, CATNAKE_X - 1 + x, '#', border);
        vga_putchar_at(CATNAKE_Y + CATNAKE_HEIGHT, CATNAKE_X - 1 + x, '#', border);
    }
    for (int y = 0; y < CATNAKE_HEIGHT; y++) {
        vga_putchar_at(CATNAKE_Y + y, CATNAKE_X - 1, '#', border);
        vga_putchar_at(CATNAKE_Y + y, CATNAKE_X + CATNAKE_WIDTH, '#', border);
    }
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
    for (;;) {
        f.x = rng_next() % CATNAKE_WIDTH;
        f.y = rng_next() % CATNAKE_HEIGHT;
        if (!catnake_occupied(body, len, f.x, f.y)) return f;
    }
}

static void catnake_render_all(const pt_t *body, int len, pt_t food, int score) {
    uint8_t bg = VGA_ATTR(VGA_WHITE, VGA_BLACK);
    for (int y = 0; y < CATNAKE_HEIGHT; y++)
        for (int x = 0; x < CATNAKE_WIDTH; x++)
            catnake_draw_cell(x, y, ' ', bg);
    catnake_draw_border();
    vga_write_at(1, 2, "Catnake  (WASD/Arrows)  Esc=quit", VGA_ATTR(VGA_WHITE, VGA_BLACK));
    vga_write_at(1, 55, "Score:", VGA_ATTR(VGA_WHITE, VGA_BLACK));
    char sbuf[12];
    itoa(score, sbuf, 10);
    vga_write_at(1, 62, sbuf, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
    catnake_draw_cell(food.x, food.y, '*', VGA_ATTR(VGA_YELLOW, VGA_BLACK));
    for (int i = len - 1; i >= 0; i--) {
        char ch;
        uint8_t attr;
        if (i == 0) { ch = 'C'; attr = VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK); }  // cat head
        else if (i == 1) { ch = '>'; attr = VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK); }  // neck/shoulder
        else ch = 'o', attr = VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK);  // body
        catnake_draw_cell(body[i].x, body[i].y, ch, attr);
    }
}

void catnake_run() {
    for (;;) {
        catnake_rng = 0xC0FFEE11u;
        pt_t body[CATNAKE_MAX];
        int len = 4;
        int dirx = 1, diry = 0;
        int pending_dirx = 1, pending_diry = 0;
        int score = 0;
        int sx = CATNAKE_WIDTH / 2, sy = CATNAKE_HEIGHT / 2;
        for (int i = 0; i < len; i++) { body[i].x = sx - i; body[i].y = sy; }
        pt_t food = catnake_place_food(body, len);
        vga_clear(VGA_ATTR(VGA_WHITE, VGA_BLACK));
        bool dead = false;
        for (;;) {
            int k;
            while ((k = keyboard_poll()) != 0) pending_input = k;
            if (pending_input == KEY_ESCAPE) break;
            if (!dead) {
                if (pending_input) {
                    if (pending_input == KEY_UP || pending_input == 'w' || pending_input == 'W') { pending_dirx = 0; pending_diry = -1; }
                    if (pending_input == KEY_DOWN || pending_input == 's' || pending_input == 'S') { pending_dirx = 0; pending_diry = 1; }
                    if (pending_input == KEY_LEFT || pending_input == 'a' || pending_input == 'A') { pending_dirx = -1; pending_diry = 0; }
                    if (pending_input == KEY_RIGHT || pending_input == 'd' || pending_input == 'D') { pending_dirx = 1; pending_diry = 0; }
                    pending_input = 0;
                }
                if (!(pending_dirx == -dirx && pending_diry == -diry)) { dirx = pending_dirx; diry = pending_diry; }
                pt_t next = { body[0].x + dirx, body[0].y + diry };
                if (next.x < 0 || next.x >= CATNAKE_WIDTH || next.y < 0 || next.y >= CATNAKE_HEIGHT) dead = true;
                else if (catnake_occupied(body, len, next.x, next.y)) dead = true;
                else {
                    for (int i = len - 1; i > 0; i--) body[i] = body[i - 1];
                    body[0] = next;
                    if (body[0].x == food.x && body[0].y == food.y) {
                        if (len < CATNAKE_MAX) { body[len] = body[len - 1]; len++; }
                        score++;
                        food = catnake_place_food(body, len);
                    }
                }
            } else {
                vga_write_at(10, 30, "Game Over! Press Enter to restart", VGA_ATTR(VGA_RED, VGA_BLACK));
                if (pending_input == KEY_ENTER) break;
            }
            catnake_render_all(body, len, food, score);
            uint32_t delay_loops;
            if (dead) delay_loops = 70;
            else if (score < 10) delay_loops = 50;
            else if (score < 25) delay_loops = 35;
            else delay_loops = 25;
            for (uint32_t d = 0; d < delay_loops; d++)
                for (volatile uint32_t i = 0; i < 50000; i++)
                    while ((k = keyboard_poll()) != 0) pending_input = k;
        }
        if (pending_input == KEY_ESCAPE) break;
        pending_input = 0;
    }
}