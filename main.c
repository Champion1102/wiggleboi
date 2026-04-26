/*
 * Snake Game — Phase 2: Entity pool + spatial grid.
 *
 * Built entirely from scratch in C with zero standard library functions.
 * Truecolor double-buffered renderer, O(1) collision via spatial grid,
 * entity pool with stack-based allocation (OpenRCT2 pattern).
 *
 * Controls: WASD / Arrow keys / HJKL (vim), Q to quit.
 */

#include "libs/memory.h"
#include "libs/screen.h"
#include "libs/keyboard.h"
#include "libs/string.h"
#include "libs/math.h"
#include "libs/sound.h"
#include "libs/entity.h"
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>

#define WIDTH     39
#define HEIGHT    20

#define SND_FOOD     "assets/food.mp3"
#define SND_MOVE     "assets/move.mp3"
#define SND_GAMEOVER "assets/gameover.mp3"
#define SND_MUSIC    "assets/music.mp3"

/* ---- Game state ---- */

static int head_id = -1;
static int tail_id = -1;
static int food_id = -1;
static int snake_len;
static int dir_x = 1, dir_y = 0;
static int score     = 0;
static int game_over = 0;
static int tick      = 0;

/* ---- Colors ---- */

enum {
    BG_R = 12, BG_G = 12, BG_B = 20,
    BD_R = 45, BD_G = 45, BD_B = 65,
    HD_R = 50, HD_G = 255, HD_B = 120,
    FD_R = 220, FD_G = 60, FD_B = 30
};

/* ---- Snake chain helpers ---- */

static int snake_push_head(int x, int y) {
    int id = ent_alloc();
    struct Entity *e, *old;

    if (id < 0) return -1;
    e = ent_get(id);
    e->x = x;
    e->y = y;
    e->type = ENT_SNAKE_HEAD;
    e->flags = ENT_ALIVE | ENT_VISIBLE;

    if (head_id >= 0) {
        old = ent_get(head_id);
        old->type = ENT_SNAKE_BODY;
        old->chain_next = id;
        e->chain_prev = head_id;
    } else {
        tail_id = id;
    }

    head_id = id;
    grid_set(x, y, id);
    snake_len++;
    return id;
}

static void snake_pop_tail(void) {
    struct Entity *t;
    int next;

    if (tail_id < 0) return;
    t = ent_get(tail_id);
    grid_clear(t->x, t->y);
    next = t->chain_next;
    ent_free(tail_id);

    tail_id = next;
    if (tail_id >= 0) {
        ent_get(tail_id)->chain_prev = ENT_NONE;
    } else {
        head_id = -1;
    }
    snake_len--;
}

/* ---- Rendering ---- */

static void draw_snake(void) {
    int id, i, dist, maxd, r, g, b;
    struct Entity *e;

    maxd = snake_len > 1 ? snake_len - 1 : 1;
    id = tail_id;
    i = 0;

    while (id != ENT_NONE && id >= 0) {
        e = ent_get(id);
        if (e->type == ENT_SNAKE_HEAD) {
            scr_cell(e->x, e->y, HD_R, HD_G, HD_B);
        } else {
            dist = snake_len - 1 - i;
            r = 20  - (dist * 15)  / maxd;
            g = 220 - (dist * 170) / maxd;
            b = 100 - (dist * 65)  / maxd;
            scr_cell(e->x, e->y, r, g, b);
        }
        id = e->chain_next;
        i++;
    }
}

static void draw_food(void) {
    struct Entity *f;
    int phase, bright, r, g;

    if (food_id < 0) return;
    f = ent_get(food_id);
    phase  = (tick >> 2) & 7;
    bright = phase < 4 ? phase : 7 - phase;
    r = 200 + bright * 14;
    g = 40  + bright * 10;
    scr_cell(f->x, f->y, r, g, 30);
}

static void draw_hud(void) {
    char buf[16];
    int len;

    scr_text(HEIGHT + 2, 2, "Score: ", 120, 120, 160);
    len = int_to_str(score, buf);
    scr_text(HEIGHT + 2, 9, buf, 220, 220, 255);
    scr_text(HEIGHT + 2, 9 + len, "     ", 0, 0, 0);
}

static void draw_border(void) {
    int i;
    int bw = WIDTH * 2 + 2;
    int bh = HEIGHT + 2;

    for (i = 0; i < bw; i++) {
        scr_border(i, 0, BD_R, BD_G, BD_B);
        scr_border(i, bh - 1, BD_R, BD_G, BD_B);
    }
    for (i = 1; i < bh - 1; i++) {
        scr_border(0, i, BD_R, BD_G, BD_B);
        scr_border(bw - 1, i, BD_R, BD_G, BD_B);
    }
}

static void render_game(void) {
    int x, y;

    for (y = 0; y < HEIGHT; y++)
        for (x = 0; x < WIDTH; x++)
            scr_cell(x, y, BG_R, BG_G, BG_B);

    draw_food();
    draw_snake();
    draw_hud();
}

/* ---- Game logic ---- */

static void place_food(void) {
    struct Entity *f;
    int fx, fy;

    do {
        fx = rng_range(WIDTH);
        fy = rng_range(HEIGHT);
    } while (grid_occupied(fx, fy));

    if (food_id < 0) {
        food_id = ent_alloc();
    }
    f = ent_get(food_id);
    f->x = fx;
    f->y = fy;
    f->type = ENT_FOOD;
    f->flags = ENT_ALIVE | ENT_VISIBLE;
    grid_set(fx, fy, food_id);
}

static void init_game(void) {
    int i;

    ent_init(WIDTH, HEIGHT);
    seed_rng((unsigned int)time(0));

    snake_len = 0;
    head_id = -1;
    tail_id = -1;

    for (i = 0; i < 3; i++)
        snake_push_head(WIDTH / 2 - 2 + i, HEIGHT / 2);
}

static int handle_input(void) {
    int key = kb_read();
    int turned = 0;

    if (key == 'q' || key == 'Q') return 0;

    if ((key == 'w' || key == 'k' || key == KB_UP)    && dir_y !=  1) { dir_x =  0; dir_y = -1; turned = 1; }
    if ((key == 's' || key == 'j' || key == KB_DOWN)   && dir_y != -1) { dir_x =  0; dir_y =  1; turned = 1; }
    if ((key == 'a' || key == 'h' || key == KB_LEFT)   && dir_x !=  1) { dir_x = -1; dir_y =  0; turned = 1; }
    if ((key == 'd' || key == 'l' || key == KB_RIGHT)  && dir_x != -1) { dir_x =  1; dir_y =  0; turned = 1; }

    if (turned) snd_play(SND_MOVE);
    return 1;
}

static void move_snake(void) {
    struct Entity *h;
    int nx, ny, gval;

    h = ent_get(head_id);
    nx = h->x + dir_x;
    ny = h->y + dir_y;

    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) {
        game_over = 1;
        return;
    }

    gval = grid_at(nx, ny);

    if (gval != ENT_NONE && gval != food_id) {
        game_over = 1;
        return;
    }

    if (gval == food_id) {
        grid_clear(nx, ny);
        snake_push_head(nx, ny);
        score++;
        snd_play(SND_FOOD);
        place_food();
    } else {
        snake_pop_tail();
        snake_push_head(nx, ny);
    }
}

/* ---- Lifecycle ---- */

static void cleanup(void) {
    int tr, tc;

    snd_music_stop();
    snd_play(SND_GAMEOVER);

    tr = 1 + HEIGHT / 2;
    tc = 1 + (WIDTH * 2 - 9) / 2;
    scr_text(tr, tc, "GAME OVER", 255, 60, 60);
    scr_present();
    usleep(2000000);

    scr_shutdown();
    kb_restore();
}

static void on_signal(int sig) {
    (void)sig;
    snd_music_stop();
    scr_shutdown();
    kb_restore();
    _exit(0);
}

int main(void) {
    init_game();

    kb_raw();
    scr_init(WIDTH, HEIGHT);

    scr_fill(BG_R, BG_G, BG_B);
    draw_border();
    place_food();
    render_game();
    scr_present();

    atexit(snd_music_stop);
    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGHUP,  on_signal);
    signal(SIGQUIT, on_signal);

    snd_music_start(SND_MUSIC);

    while (!game_over) {
        if (!handle_input())
            break;

        move_snake();
        tick++;
        render_game();
        scr_present();

        usleep(100000);
    }

    cleanup();
    return 0;
}
