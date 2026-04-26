#include "libs/memory.h"
#include "libs/screen.h"
#include "libs/keyboard.h"
#include "libs/string.h"
#include "libs/math.h"
#include "libs/sound.h"
#include "libs/entity.h"
#include "libs/fx.h"
#include "libs/ai.h"
#include "libs/save.h"
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>

static int g_w = 39, g_h = 20;
#define TW (g_w * 2 + 2)

#define SND_FOOD     "assets/food.mp3"
#define SND_MOVE     "assets/move.mp3"
#define SND_GAMEOVER "assets/gameover.mp3"
#define SND_MUSIC    "assets/music.mp3"

/* ---- State machine ---- */

enum { ST_MENU, ST_PLAYING, ST_PAUSED, ST_GAMEOVER, ST_QUIT };

static int state = ST_MENU;
static int go_phase, go_id, go_fade;

/* ---- Game modes ---- */

enum { MODE_CLASSIC, MODE_SPEED, MODE_INFINITY, MODE_CHALLENGE, MODE_COUNT };

static int game_mode = MODE_CLASSIC;
static const char *mode_names[] = { "Classic", "Speed", "Infinity", "Challenge" };
static const char *mode_descs[] = {
    "Speed increases with score",
    "Starts fast, gets faster",
    "Walls wrap around",
    "Obstacles appear on grid"
};

/* ---- Game state ---- */

static int head_id = -1;
static int tail_id = -1;
static int food_id = -1;
static int snake_len;
static int dir_x = 1, dir_y = 0;
static int score     = 0;
static int game_over = 0;
static int tick      = 0;

/* ---- Items & power-ups ---- */

static int bonus_id = -1;
static int bonus_timer = 0;
static int bonus_next = 10;

static int poison_id = -1;

enum { PU_SHIELD = 1, PU_SPEED, PU_SLOW, PU_DOUBLE };
static int powerup_id   = -1;
static int powerup_type = 0;

static int shield_timer = 0;
static int double_timer = 0;
static int speed_timer  = 0;
static int slow_timer   = 0;

#define MAX_OBS 20
static int obs_ids[MAX_OBS];
static int obs_count = 0;
static int obs_next  = 5;

static int is_new_high = 0;

/* ---- Colors ---- */

enum {
    BG_R = 12, BG_G = 12, BG_B = 20,
    BD_R = 45, BD_G = 45, BD_B = 65,
    HD_R = 50, HD_G = 255, HD_B = 120
};

static void detect_size(void) {
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        g_w = (ws.ws_col - 2) / 2;
        g_h = ws.ws_row - 4;
    }
    if (g_w < 20) g_w = 20;
    if (g_h < 12) g_h = 12;
    if (g_w > 60) g_w = 60;
    if (g_h > 30) g_h = 30;
}

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
    if (tail_id >= 0 && tail_id != ENT_NONE) {
        ent_get(tail_id)->chain_prev = ENT_NONE;
    } else {
        tail_id = -1;
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
        if (!(e->flags & ENT_ALIVE)) {
            id = e->chain_next;
            i++;
            continue;
        }
        if (e->type == ENT_SNAKE_HEAD) {
            if (shield_timer > 0)
                scr_cell(e->x, e->y, 80, 220, 255);
            else
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
    int phase, bright, r, g, dx, dy, gx, gy;

    if (food_id < 0) return;
    f = ent_get(food_id);
    phase  = (tick >> 2) & 7;
    bright = phase < 4 ? phase : 7 - phase;
    r = 200 + bright * 14;
    g = 40  + bright * 10;

    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            gx = f->x + dx;
            gy = f->y + dy;
            if (gx >= 0 && gx < g_w && gy >= 0 && gy < g_h &&
                !grid_occupied(gx, gy))
                scr_cell(gx, gy,
                         BG_R + 18 + bright * 4,
                         BG_G + 6  + bright * 2,
                         BG_B + 3);
        }
    }

    scr_cell(f->x, f->y, r, g, 30);
}

static void draw_bonus(void) {
    struct Entity *b;
    int phase, bright, dx, dy, gx, gy;

    if (bonus_id < 0) return;
    b = ent_get(bonus_id);

    if (bonus_timer > 0 && bonus_timer < 20 && (tick & 3) < 2)
        return;

    phase = (tick >> 1) & 7;
    bright = phase < 4 ? phase : 7 - phase;

    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            gx = b->x + dx;
            gy = b->y + dy;
            if (gx >= 0 && gx < g_w && gy >= 0 && gy < g_h &&
                !grid_occupied(gx, gy))
                scr_cell(gx, gy,
                         BG_R + 25 + bright * 5,
                         BG_G + 20 + bright * 4,
                         BG_B);
        }
    }

    scr_cell(b->x, b->y, 255, 220 + bright * 8, 30 + bright * 8);
}

static void draw_poison(void) {
    struct Entity *p;
    int phase, bright;

    if (poison_id < 0) return;
    p = ent_get(poison_id);
    phase = (tick >> 2) & 7;
    bright = phase < 4 ? phase : 7 - phase;
    scr_cell(p->x, p->y, 160 + bright * 12, 30, 180 + bright * 10);
}

static void draw_powerup(void) {
    struct Entity *p;
    int phase, bright, r, g, b, dx, dy, gx, gy;

    if (powerup_id < 0) return;
    p = ent_get(powerup_id);

    switch (powerup_type) {
    case PU_SHIELD:
        phase = (tick >> 1) & 7;
        bright = phase < 4 ? phase : 7 - phase;
        r = 40; g = 160 + bright * 12; b = 230 + bright * 6;
        break;
    case PU_SPEED:
        phase = tick & 7;
        bright = phase < 4 ? phase : 7 - phase;
        r = 240 + bright * 4; g = 200 + bright * 14; b = 30;
        break;
    case PU_SLOW:
        phase = (tick >> 2) & 7;
        bright = phase < 4 ? phase : 7 - phase;
        r = 80 + bright * 10; g = 180 + bright * 10; b = 240 + bright * 4;
        break;
    case PU_DOUBLE:
        phase = (tick >> 1) & 7;
        bright = phase < 4 ? phase : 7 - phase;
        r = 255; g = 180 + bright * 10; b = 20 + bright * 5;
        break;
    default: return;
    }

    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            gx = p->x + dx;
            gy = p->y + dy;
            if (gx >= 0 && gx < g_w && gy >= 0 && gy < g_h &&
                !grid_occupied(gx, gy))
                scr_cell(gx, gy, BG_R + r / 15, BG_G + g / 15, BG_B + b / 15);
        }
    }

    scr_cell(p->x, p->y, r, g, b);
}

static void draw_obstacles(void) {
    int i, phase, bright;
    struct Entity *o;

    phase = (tick >> 3) & 7;
    bright = phase < 4 ? phase : 7 - phase;

    for (i = 0; i < obs_count; i++) {
        o = ent_get(obs_ids[i]);
        scr_cell(o->x, o->y, 55 + bright, 50 + bright, 45 + bright);
    }
}

static void draw_hud(void) {
    char buf[16];
    int len, col;

    scr_text(g_h + 2, 2, "Score:", 120, 120, 160);
    len = int_to_str(score, buf);
    scr_text(g_h + 2, 9, buf, 220, 220, 255);
    scr_text(g_h + 2, 9 + len, "  ", 0, 0, 0);

    col = 14;
    if (shield_timer > 0) { scr_text(g_h + 2, col, "SHD", 60, 200, 255); col += 4; }
    if (speed_timer > 0)  { scr_text(g_h + 2, col, "SPD", 255, 220, 50);  col += 4; }
    if (slow_timer > 0)   { scr_text(g_h + 2, col, "SLO", 100, 220, 255); col += 4; }
    if (double_timer > 0) { scr_text(g_h + 2, col, "x2",  255, 200, 30);  col += 3; }
    while (col < TW - 12) { scr_text(g_h + 2, col, " ", 0, 0, 0); col++; }

    scr_text(g_h + 2, TW - 10, "P:Pause", 80, 80, 110);
}

static void draw_border(void) {
    int i, bw, bh, phase, bright, br, bg, bb;

    bw = TW;
    bh = g_h + 2;
    phase = (tick >> 1) & 31;
    bright = phase < 16 ? phase : 31 - phase;
    br = BD_R + bright;
    bg = BD_G + bright;
    bb = BD_B + bright * 2;

    if (game_mode == MODE_INFINITY && state == ST_PLAYING) {
        br += 10; bg -= 5; bb += 15;
    }

    for (i = 0; i < bw; i++) {
        scr_border(i, 0, br, bg, bb);
        scr_border(i, bh - 1, br, bg, bb);
    }
    for (i = 1; i < bh - 1; i++) {
        scr_border(0, i, br, bg, bb);
        scr_border(bw - 1, i, br, bg, bb);
    }
}

static void render_game(void) {
    int x, y;

    for (y = 0; y < g_h; y++)
        for (x = 0; x < g_w; x++)
            scr_cell(x, y, BG_R, BG_G, BG_B);

    fx_draw_trail();
    draw_obstacles();
    draw_food();
    draw_bonus();
    draw_poison();
    draw_powerup();
    draw_snake();
    ai_draw(tick);
    fx_draw_parts();
    draw_border();
    draw_hud();
}

/* ---- Item spawning ---- */

static void spawn_at_empty(int *out_x, int *out_y) {
    do {
        *out_x = rng_range(g_w);
        *out_y = rng_range(g_h);
    } while (grid_occupied(*out_x, *out_y));
}

static void place_food(void) {
    struct Entity *f;
    int fx, fy;

    spawn_at_empty(&fx, &fy);
    if (food_id < 0)
        food_id = ent_alloc();
    f = ent_get(food_id);
    f->x = fx;
    f->y = fy;
    f->type = ENT_FOOD;
    f->flags = ENT_ALIVE | ENT_VISIBLE;
    grid_set(fx, fy, food_id);
}

static void spawn_bonus(void) {
    struct Entity *f;
    int fx, fy;

    if (bonus_id >= 0) return;
    spawn_at_empty(&fx, &fy);
    bonus_id = ent_alloc();
    if (bonus_id < 0) return;
    f = ent_get(bonus_id);
    f->x = fx;
    f->y = fy;
    f->type = ENT_BONUS;
    f->flags = ENT_ALIVE | ENT_VISIBLE;
    grid_set(fx, fy, bonus_id);
    bonus_timer = 80;
}

static void despawn_bonus(void) {
    if (bonus_id < 0) return;
    grid_clear(ent_get(bonus_id)->x, ent_get(bonus_id)->y);
    ent_free(bonus_id);
    bonus_id = -1;
    bonus_timer = 0;
}

static void spawn_poison(void) {
    struct Entity *f;
    int fx, fy;

    if (poison_id >= 0) return;
    spawn_at_empty(&fx, &fy);
    poison_id = ent_alloc();
    if (poison_id < 0) return;
    f = ent_get(poison_id);
    f->x = fx;
    f->y = fy;
    f->type = ENT_POISON;
    f->flags = ENT_ALIVE | ENT_VISIBLE;
    grid_set(fx, fy, poison_id);
}

static void despawn_poison(void) {
    if (poison_id < 0) return;
    grid_clear(ent_get(poison_id)->x, ent_get(poison_id)->y);
    ent_free(poison_id);
    poison_id = -1;
}

static void spawn_powerup(void) {
    struct Entity *f;
    int fx, fy;

    if (powerup_id >= 0) return;
    spawn_at_empty(&fx, &fy);
    powerup_id = ent_alloc();
    if (powerup_id < 0) return;
    f = ent_get(powerup_id);
    f->x = fx;
    f->y = fy;
    f->type = ENT_POWERUP;
    f->flags = ENT_ALIVE | ENT_VISIBLE;
    grid_set(fx, fy, powerup_id);
    powerup_type = 1 + rng_range(4);
}

static void despawn_powerup(void) {
    if (powerup_id < 0) return;
    grid_clear(ent_get(powerup_id)->x, ent_get(powerup_id)->y);
    ent_free(powerup_id);
    powerup_id = -1;
    powerup_type = 0;
}

static void spawn_obstacle(void) {
    struct Entity *o;
    int fx, fy, id;

    if (obs_count >= MAX_OBS) return;
    spawn_at_empty(&fx, &fy);
    id = ent_alloc();
    if (id < 0) return;
    o = ent_get(id);
    o->x = fx;
    o->y = fy;
    o->type = ENT_OBSTACLE;
    o->flags = ENT_ALIVE | ENT_VISIBLE;
    grid_set(fx, fy, id);
    obs_ids[obs_count++] = id;
}

/* ---- Game logic ---- */

static void reset_game(void) {
    int i;

    ent_init(g_w, g_h);
    fx_init();
    ai_init(g_w, g_h);

    head_id = -1;
    tail_id = -1;
    food_id = -1;
    bonus_id = -1;
    poison_id = -1;
    powerup_id = -1;
    powerup_type = 0;
    bonus_timer = 0;
    bonus_next = 10;
    shield_timer = 0;
    double_timer = 0;
    speed_timer = 0;
    slow_timer = 0;
    obs_count = 0;
    obs_next = 5;
    snake_len = 0;
    dir_x = 1;
    dir_y = 0;
    score = 0;
    game_over = 0;

    for (i = 0; i < 3; i++)
        snake_push_head(g_w / 2 - 2 + i, g_h / 2);
}

static void process_direction(int key) {
    int turned = 0;

    if ((key == 'w' || key == 'k' || key == KB_UP)    && dir_y !=  1) { dir_x =  0; dir_y = -1; turned = 1; }
    if ((key == 's' || key == 'j' || key == KB_DOWN)   && dir_y != -1) { dir_x =  0; dir_y =  1; turned = 1; }
    if ((key == 'a' || key == 'h' || key == KB_LEFT)   && dir_x !=  1) { dir_x = -1; dir_y =  0; turned = 1; }
    if ((key == 'd' || key == 'l' || key == KB_RIGHT)  && dir_x != -1) { dir_x =  1; dir_y =  0; turned = 1; }

    if (turned) snd_play(SND_MOVE);
}

static void move_snake(void) {
    struct Entity *h, *t;
    int nx, ny, gval, pts, n;

    h = ent_get(head_id);
    nx = h->x + dir_x;
    ny = h->y + dir_y;

    /* Wall handling */
    if (game_mode == MODE_INFINITY) {
        if (nx < 0) nx = g_w - 1;
        else if (nx >= g_w) nx = 0;
        if (ny < 0) ny = g_h - 1;
        else if (ny >= g_h) ny = 0;
    } else if (nx < 0 || nx >= g_w || ny < 0 || ny >= g_h) {
        if (shield_timer > 0) {
            shield_timer = 0;
            dir_x = -dir_x; dir_y = -dir_y;
            fx_spawn_burst(h->x, h->y, 60, 180, 255, 8);
            return;
        }
        game_over = 1;
        fx_shake(8);
        fx_spawn_burst(h->x, h->y, 255, 60, 40, 12);
        return;
    }

    gval = grid_at(nx, ny);

    /* Empty cell */
    if (gval == ENT_NONE) {
        t = ent_get(tail_id);
        fx_trail_push(t->x, t->y);
        snake_pop_tail();
        snake_push_head(nx, ny);
        return;
    }

    /* Regular food */
    if (gval == food_id) {
        grid_clear(nx, ny);
        snake_push_head(nx, ny);
        pts = double_timer > 0 ? 2 : 1;
        score += pts;
        snd_play(SND_FOOD);
        fx_spawn_eat(nx, ny);
        place_food();
        return;
    }

    /* Bonus food */
    if (bonus_id >= 0 && gval == bonus_id) {
        grid_clear(nx, ny);
        ent_free(bonus_id);
        bonus_id = -1;
        bonus_timer = 0;
        snake_push_head(nx, ny);
        pts = double_timer > 0 ? 6 : 3;
        score += pts;
        snd_play(SND_FOOD);
        fx_spawn_eat(nx, ny);
        return;
    }

    /* Poison food */
    if (poison_id >= 0 && gval == poison_id) {
        grid_clear(nx, ny);
        ent_free(poison_id);
        poison_id = -1;
        n = 3;
        while (n-- > 0 && snake_len > 2) {
            t = ent_get(tail_id);
            fx_spawn_burst(t->x, t->y, 160, 30, 180, 2);
            snake_pop_tail();
        }
        t = ent_get(tail_id);
        fx_trail_push(t->x, t->y);
        snake_pop_tail();
        snake_push_head(nx, ny);
        fx_shake(3);
        return;
    }

    /* Power-up */
    if (powerup_id >= 0 && gval == powerup_id) {
        grid_clear(nx, ny);
        switch (powerup_type) {
        case PU_SHIELD: shield_timer = 50; break;
        case PU_SPEED:  speed_timer = 50;  break;
        case PU_SLOW:   slow_timer = 50;   break;
        case PU_DOUBLE: double_timer = 100; break;
        }
        ent_free(powerup_id);
        powerup_id = -1;
        powerup_type = 0;
        snd_play(SND_FOOD);
        fx_spawn_burst(nx, ny, 100, 200, 255, 6);
        t = ent_get(tail_id);
        fx_trail_push(t->x, t->y);
        snake_pop_tail();
        snake_push_head(nx, ny);
        return;
    }

    /* Collision — body or obstacle */
    if (shield_timer > 0) {
        shield_timer = 0;
        dir_x = -dir_x; dir_y = -dir_y;
        fx_spawn_burst(nx, ny, 60, 180, 255, 8);
        return;
    }
    game_over = 1;
    fx_shake(8);
    fx_spawn_burst(nx, ny, 255, 60, 40, 12);
}

static void update_items(void) {
    if (bonus_timer > 0) {
        bonus_timer--;
        if (bonus_timer <= 0)
            despawn_bonus();
    }

    if (bonus_id < 0 && score >= bonus_next) {
        spawn_bonus();
        bonus_next = score + 10;
    }

    if (poison_id < 0 && score > 5 && rng_range(120) == 0)
        spawn_poison();

    if (powerup_id < 0 && rng_range(180) == 0)
        spawn_powerup();

    if (shield_timer > 0) shield_timer--;
    if (double_timer > 0) double_timer--;
    if (speed_timer > 0)  speed_timer--;
    if (slow_timer > 0)   slow_timer--;

    if (game_mode == MODE_CHALLENGE && score >= obs_next && obs_count < MAX_OBS) {
        spawn_obstacle();
        obs_next = score + 5;
    }
}

static int get_delay(void) {
    int d;

    switch (game_mode) {
    case MODE_SPEED:
        d = 70000 - score * 1500;
        break;
    default:
        d = 100000 - score * 1500;
        break;
    }

    if (speed_timer > 0) d = d * 2 / 3;
    if (slow_timer > 0)  d = d * 3 / 2;

    if (d < 30000) d = 30000;
    if (d > 150000) d = 150000;
    return d;
}

/* ---- Menu ---- */

static void render_menu(void) {
    int x, y, i, col, sx, sy, hx, len, pw, mc, mlen;
    int r, g, b;
    const char *title = "WIGGLEBOI";
    char ch[2] = {0, 0};

    for (y = 0; y < g_h; y++)
        for (x = 0; x < g_w; x++)
            scr_cell(x, y, BG_R, BG_G, BG_B);

    /* Animated demo snake */
    len = 8;
    sy = g_h - 2;
    hx = (tick / 2) % (g_w + len);
    for (i = 0; i < len; i++) {
        sx = hx - i;
        if (sx >= 0 && sx < g_w) {
            if (i == 0) {
                scr_cell(sx, sy, HD_R, HD_G, HD_B);
            } else {
                r = 20 + (len - 1 - i) * 5;
                g = 220 - (len - 1 - i) * 25;
                b = 100 - (len - 1 - i) * 10;
                scr_cell(sx, sy, r, g, b);
            }
        }
    }

    /* Title with wave shimmer */
    col = (TW - 17) / 2;
    for (i = 0; i < 9; i++) {
        ch[0] = title[i];
        pw = ((tick + i * 7) & 31);
        pw = pw < 16 ? pw : 31 - pw;
        scr_text(4, col + i * 2, ch,
                 30 + pw * 3,
                 200 + pw * 3,
                 100 + pw * 5 + i * 8);
    }

    /* Mode selector */
    mlen = str_len(mode_names[game_mode]);
    mc = (TW - mlen - 4) / 2;
    scr_text(8, mc, "< ", 100, 100, 140);
    scr_text(8, mc + 2, mode_names[game_mode], 200, 230, 200);
    scr_text(8, mc + 2 + mlen, " >", 100, 100, 140);
    scr_text(8, mc + 4 + mlen, "      ", 0, 0, 0);

    /* Mode description */
    mlen = str_len(mode_descs[game_mode]);
    scr_text(9, (TW - mlen) / 2, mode_descs[game_mode], 80, 90, 100);

    scr_text(11, (TW - 12) / 2, "[ENTER] Play", 150, 200, 150);
    scr_text(12, (TW - 8) / 2, "[Q] Quit", 150, 150, 180);

    /* High scores */
    if (save_count() > 0 && g_h >= 18) {
        int sc, si, slen, srow;
        char sbuf[16];
        struct ScoreEntry *se;

        srow = 14;
        scr_text(srow, (TW - 11) / 2, "HIGH SCORES", 130, 130, 170);
        for (si = 0; si < save_count() && si < 3; si++) {
            se = save_get(si);
            sc = (TW - 20) / 2;
            sbuf[0] = '1' + si;
            sbuf[1] = '.';
            sbuf[2] = ' ';
            sbuf[3] = 0;
            scr_text(srow + 1 + si, sc, sbuf, 100, 100, 140);
            sc += 3;
            slen = int_to_str(se->score, sbuf);
            scr_text(srow + 1 + si, sc, sbuf, 200, 200, 230);
            sc += slen + 1;
            if (se->mode >= 0 && se->mode < MODE_COUNT)
                scr_text(srow + 1 + si, sc, mode_names[se->mode], 80, 90, 110);
        }
    }

    scr_text(g_h + 2, 2, "WASD / Arrows / HJKL  P:Pause", 80, 80, 110);

    draw_border();
}

/* ---- State handlers ---- */

static void tick_menu(void) {
    int key = kb_read();

    if (key == 'q' || key == 'Q') { state = ST_QUIT; return; }
    if (key == '\n' || key == '\r' || key == ' ') {
        reset_game();
        scr_fill(BG_R, BG_G, BG_B);
        place_food();
        state = ST_PLAYING;
        return;
    }
    if (key == 'a' || key == 'h' || key == KB_LEFT)
        game_mode = (game_mode + MODE_COUNT - 1) % MODE_COUNT;
    if (key == 'd' || key == 'l' || key == KB_RIGHT)
        game_mode = (game_mode + 1) % MODE_COUNT;

    tick++;
    render_menu();
    scr_present();
    usleep(100000);
}

static void tick_playing(void) {
    int key = kb_read();

    if (key == 'q' || key == 'Q') {
        scr_fill(BG_R, BG_G, BG_B);
        state = ST_MENU;
        return;
    }
    if (key == 'p' || key == 'P') {
        state = ST_PAUSED;
        render_game();
        scr_text(1 + g_h / 2,     (TW - 6) / 2,  "PAUSED",              255, 255, 200);
        scr_text(1 + g_h / 2 + 2, (TW - 20) / 2, "P - Resume  Q - Menu", 150, 150, 180);
        scr_present();
        return;
    }

    process_direction(key);
    move_snake();

    if (!game_over) {
        struct Entity *fh = ent_get(head_id);
        struct Entity *ff = food_id >= 0 ? ent_get(food_id) : 0;
        ai_update(tick,
                  ff ? ff->x : -1, ff ? ff->y : -1,
                  fh->x, fh->y, score);
        if (ai_ate_food())
            place_food();
    }

    if (game_over) {
        snd_play(SND_GAMEOVER);
        despawn_bonus();
        despawn_poison();
        despawn_powerup();
        ai_kill_all();
        is_new_high = save_is_high(score);
        save_add(score, game_mode);
        state = ST_GAMEOVER;
        go_phase = 0;
        go_id = tail_id;
        go_fade = 0;
    }

    tick++;
    update_items();
    fx_update();
    render_game();
    scr_present();
    usleep(get_delay());
}

static void tick_paused(void) {
    int key = kb_read();

    if (key == 'p' || key == 'P') { state = ST_PLAYING; return; }
    if (key == 'q' || key == 'Q') {
        scr_fill(BG_R, BG_G, BG_B);
        state = ST_MENU;
        return;
    }
    usleep(50000);
}

static void tick_gameover(void) {
    int key, tr, tc, oc;
    struct Entity *e;
    char nbuf[16];

    key = kb_read();
    if (key == '\n' || key == '\r' || key == ' ') {
        snd_music_start(SND_MUSIC);
        reset_game();
        scr_fill(BG_R, BG_G, BG_B);
        place_food();
        state = ST_PLAYING;
        return;
    }
    if (key == 'q' || key == 'Q') {
        snd_music_start(SND_MUSIC);
        scr_fill(BG_R, BG_G, BG_B);
        state = ST_MENU;
        return;
    }

    if (go_phase == 0) {
        if (go_id != ENT_NONE && go_id >= 0) {
            e = ent_get(go_id);
            go_id = e->chain_next;
            fx_spawn_burst(e->x, e->y, 255, 50 + rng_range(40), 30, 4);
            grid_clear(e->x, e->y);
            e->flags = 0;
        } else {
            head_id = -1;
            tail_id = -1;
            snake_len = 0;
            if (food_id >= 0) {
                grid_clear(ent_get(food_id)->x, ent_get(food_id)->y);
                food_id = -1;
            }
            go_phase = 1;
            go_fade = 20;
            snd_music_stop();
        }

        tick++;
        fx_update();
        render_game();
        scr_present();
        usleep(35000);

    } else if (go_phase == 1) {
        tick++;
        fx_update();
        render_game();
        scr_present();
        if (--go_fade <= 0) go_phase = 2;
        usleep(40000);

    } else {
        tick++;
        render_game();

        tr = 1 + g_h / 2 - 2;
        tc = (TW - 9) / 2;
        scr_text(tr, tc, mode_names[game_mode], 80, 120, 160);

        scr_text(tr + 1, tc, "GAME OVER", 255, 60, 60);

        scr_text(tr + 3, tc, "Score: ", 120, 120, 160);
        int_to_str(score, nbuf);
        scr_text(tr + 3, tc + 7, nbuf, 220, 220, 255);

        if (is_new_high) {
            oc = (TW - 16) / 2;
            scr_text(tr + 4, oc, "NEW HIGH SCORE!", 255, 220, 50);
        }

        oc = (TW - 23) / 2;
        scr_text(tr + 6, oc, "[ENTER] Again  [Q] Menu", 150, 150, 180);

        scr_present();
        usleep(100000);
    }
}

/* ---- Lifecycle ---- */

static void on_signal(int sig) {
    (void)sig;
    snd_music_stop();
    scr_shutdown();
    kb_restore();
    _exit(0);
}

int main(void) {
    seed_rng((unsigned int)time(0));
    detect_size();
    save_load();
    kb_raw();
    scr_init(g_w, g_h);
    scr_fill(BG_R, BG_G, BG_B);
    snd_music_start(SND_MUSIC);

    atexit(snd_music_stop);
    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGHUP,  on_signal);
    signal(SIGQUIT, on_signal);

    while (state != ST_QUIT) {
        switch (state) {
        case ST_MENU:     tick_menu();     break;
        case ST_PLAYING:  tick_playing();  break;
        case ST_PAUSED:   tick_paused();   break;
        case ST_GAMEOVER: tick_gameover(); break;
        default: break;
        }
    }

    snd_music_stop();
    scr_shutdown();
    kb_restore();
    return 0;
}
