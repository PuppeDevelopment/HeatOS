#include "terminal.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"

#define PONG_LEFT 6
#define PONG_TOP 2
#define PONG_WIDTH 68
#define PONG_HEIGHT 20
#define PONG_PADDLE_H 4
#define PONG_WIN_SCORE 7

#define PONG_BASE_DELAY_LOOPS 3000000
#define PONG_MIN_DELAY_LOOPS 1500000
#define PONG_SPEEDUP_TICKS 220
#define PONG_SPEEDUP_STEP 70000
#define PONG_MAX_SPEED_STAGE 20

#define PONG_BASE_MOVE_STRIDE 6
#define PONG_MIN_MOVE_STRIDE 2

static int pong_clamp_paddle(int y) {
    if (y < 0) return 0;
    if (y > (PONG_HEIGHT - PONG_PADDLE_H)) return (PONG_HEIGHT - PONG_PADDLE_H);
    return y;
}

static void pong_draw_scene(int player_y, int ai_y, int ball_x, int ball_y, int player_score, int ai_score) {
    uint8_t text_attr = VGA_ATTR(VGA_WHITE, VGA_BLACK);
    uint8_t accent_attr = VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK);

    vga_clear(text_attr);
    vga_write_at(0, 2, "PONG  (W/S or Arrows)  Esc=Quit", accent_attr);

    char nbuf[16];
    vga_write_at(0, 44, "You:", text_attr);
    itoa(player_score, nbuf, 10);
    vga_write_at(0, 49, nbuf, VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK));
    vga_write_at(0, 55, "CPU:", text_attr);
    itoa(ai_score, nbuf, 10);
    vga_write_at(0, 60, nbuf, VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK));

    for (int x = 0; x < PONG_WIDTH + 2; x++) {
        vga_putchar_at(PONG_TOP - 1, PONG_LEFT - 1 + x, '=', VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK));
        vga_putchar_at(PONG_TOP + PONG_HEIGHT, PONG_LEFT - 1 + x, '=', VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK));
    }

    for (int y = 0; y < PONG_HEIGHT; y++) {
        vga_putchar_at(PONG_TOP + y, PONG_LEFT - 1, '|', VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK));
        vga_putchar_at(PONG_TOP + y, PONG_LEFT + PONG_WIDTH, '|', VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK));
        if ((y % 2) == 0) {
            vga_putchar_at(PONG_TOP + y, PONG_LEFT + (PONG_WIDTH / 2), ':', VGA_ATTR(VGA_DARK_GRAY, VGA_BLACK));
        }
    }

    int player_x = 2;
    int ai_x = PONG_WIDTH - 3;
    for (int i = 0; i < PONG_PADDLE_H; i++) {
        vga_putchar_at(PONG_TOP + player_y + i, PONG_LEFT + player_x, '#', VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK));
        vga_putchar_at(PONG_TOP + ai_y + i, PONG_LEFT + ai_x, '#', VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK));
    }

    vga_putchar_at(PONG_TOP + ball_y, PONG_LEFT + ball_x, 'O', VGA_ATTR(VGA_YELLOW, VGA_BLACK));
}

static void pong_wait_serve(int player_score, int ai_score) {
    pong_draw_scene(PONG_HEIGHT / 2 - 1, PONG_HEIGHT / 2 - 1, PONG_WIDTH / 2, PONG_HEIGHT / 2, player_score, ai_score);
    vga_write_at(PONG_TOP + PONG_HEIGHT / 2, PONG_LEFT + (PONG_WIDTH / 2) - 12,
                 "Next serve in 1 second...", VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK));

    for (volatile uint32_t i = 0; i < 65000000u; i++) {
        (void)i;
    }
}

void cmd_pong(const char *args) {
    (void)args;

    int player_score = 0;
    int ai_score = 0;

    for (;;) {
        int player_y = (PONG_HEIGHT / 2) - (PONG_PADDLE_H / 2);
        int ai_y = player_y;
        int ball_x = PONG_WIDTH / 2;
        int ball_y = PONG_HEIGHT / 2;
        int ball_dx = ((player_score + ai_score) & 1) ? 1 : -1;
        int ball_dy = ((player_score + ai_score) & 2) ? 1 : -1;
        int rally_ticks = 0;

        pong_wait_serve(player_score, ai_score);

        for (;;) {
            int k;
            while ((k = keyboard_poll()) != 0) {
                if (k == KEY_ESCAPE) {
                    term_reset_screen();
                    return;
                }
                if (k == KEY_UP || k == 'w' || k == 'W') player_y--;
                if (k == KEY_DOWN || k == 's' || k == 'S') player_y++;
            }

            player_y = pong_clamp_paddle(player_y);

            if (ball_y < ai_y + (PONG_PADDLE_H / 2)) ai_y--;
            else if (ball_y > ai_y + (PONG_PADDLE_H / 2)) ai_y++;
            ai_y = pong_clamp_paddle(ai_y);

            rally_ticks++;

            int speed_stage = rally_ticks / PONG_SPEEDUP_TICKS;
            if (speed_stage > PONG_MAX_SPEED_STAGE) speed_stage = PONG_MAX_SPEED_STAGE;

            int move_stride = PONG_BASE_MOVE_STRIDE - (speed_stage / 4);
            if (move_stride < PONG_MIN_MOVE_STRIDE) move_stride = PONG_MIN_MOVE_STRIDE;

            if ((rally_ticks % move_stride) == 0) {
                ball_x += ball_dx;
                ball_y += ball_dy;

                if (ball_y <= 0) {
                    ball_y = 0;
                    ball_dy = 1;
                }
                if (ball_y >= PONG_HEIGHT - 1) {
                    ball_y = PONG_HEIGHT - 1;
                    ball_dy = -1;
                }

                int player_x = 2;
                int ai_x = PONG_WIDTH - 3;

                if (ball_dx < 0 && ball_x == player_x + 1) {
                    if (ball_y >= player_y && ball_y < player_y + PONG_PADDLE_H) {
                        ball_dx = 1;
                        if (ball_y < player_y + (PONG_PADDLE_H / 2)) ball_dy = -1;
                        else ball_dy = 1;
                    }
                }

                if (ball_dx > 0 && ball_x == ai_x - 1) {
                    if (ball_y >= ai_y && ball_y < ai_y + PONG_PADDLE_H) {
                        ball_dx = -1;
                        if (ball_y < ai_y + (PONG_PADDLE_H / 2)) ball_dy = -1;
                        else ball_dy = 1;
                    }
                }

                if (ball_x < 0) {
                    ai_score++;
                    break;
                }
                if (ball_x >= PONG_WIDTH) {
                    player_score++;
                    break;
                }
            }

            pong_draw_scene(player_y, ai_y, ball_x, ball_y, player_score, ai_score);

            int delay_loops = PONG_BASE_DELAY_LOOPS - (speed_stage * PONG_SPEEDUP_STEP);
            if (delay_loops < PONG_MIN_DELAY_LOOPS) delay_loops = PONG_MIN_DELAY_LOOPS;

            for (volatile uint32_t d = 0; d < (uint32_t)delay_loops; d++) {
                (void)d;
            }
        }

        if (player_score >= PONG_WIN_SCORE || ai_score >= PONG_WIN_SCORE) {
            pong_draw_scene((PONG_HEIGHT / 2) - (PONG_PADDLE_H / 2), (PONG_HEIGHT / 2) - (PONG_PADDLE_H / 2),
                            PONG_WIDTH / 2, PONG_HEIGHT / 2, player_score, ai_score);
            if (player_score > ai_score) {
                vga_write_at(PONG_TOP + PONG_HEIGHT / 2 - 1, PONG_LEFT + (PONG_WIDTH / 2) - 6,
                             "YOU WIN!", VGA_ATTR(VGA_WHITE, VGA_GREEN));
            } else {
                vga_write_at(PONG_TOP + PONG_HEIGHT / 2 - 1, PONG_LEFT + (PONG_WIDTH / 2) - 7,
                             "CPU WINS!", VGA_ATTR(VGA_WHITE, VGA_RED));
            }
            vga_write_at(PONG_TOP + PONG_HEIGHT / 2 + 1, PONG_LEFT + (PONG_WIDTH / 2) - 16,
                         "Enter=Play Again  Esc=Quit", VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK));

            for (;;) {
                int k = keyboard_wait();
                if (k == KEY_ESCAPE) {
                    term_reset_screen();
                    return;
                }
                if (k == KEY_ENTER) {
                    player_score = 0;
                    ai_score = 0;
                    break;
                }
            }
        }
    }
}
