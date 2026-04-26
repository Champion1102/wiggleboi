#include "fx.h"
#include "screen.h"
#include "math.h"

#define PART_MAX  64
#define TRAIL_LEN 3

struct Particle {
    short x, y;
    short vx, vy;
    unsigned char r, g, b;
    unsigned char life, max_life;
};

static struct Particle parts[PART_MAX];
static int shake_timer;
static int trail_gx[TRAIL_LEN], trail_gy[TRAIL_LEN];

void fx_init(void) {
    int i;
    for (i = 0; i < PART_MAX; i++)
        parts[i].life = 0;
    shake_timer = 0;
    fx_trail_clear();
}

void fx_trail_push(int gx, int gy) {
    int i;
    for (i = TRAIL_LEN - 1; i > 0; i--) {
        trail_gx[i] = trail_gx[i - 1];
        trail_gy[i] = trail_gy[i - 1];
    }
    trail_gx[0] = gx;
    trail_gy[0] = gy;
}

void fx_trail_clear(void) {
    int i;
    for (i = 0; i < TRAIL_LEN; i++) {
        trail_gx[i] = -1;
        trail_gy[i] = -1;
    }
}

static void spawn(int gx, int gy, int r, int g, int b,
                   int vscale, int life_base, int life_var)
{
    int i;
    for (i = 0; i < PART_MAX; i++) {
        if (parts[i].life == 0) {
            parts[i].x  = gx << 4;
            parts[i].y  = gy << 4;
            parts[i].vx = rng_range(2 * vscale + 1) - vscale;
            parts[i].vy = rng_range(2 * vscale + 1) - vscale;
            parts[i].r  = (unsigned char)r;
            parts[i].g  = (unsigned char)g;
            parts[i].b  = (unsigned char)b;
            parts[i].life     = (unsigned char)(life_base + rng_range(life_var + 1));
            parts[i].max_life = parts[i].life;
            return;
        }
    }
}

void fx_spawn_eat(int gx, int gy) {
    int n;
    for (n = 0; n < 10; n++)
        spawn(gx, gy,
              255, 180 + rng_range(76), 30 + rng_range(50),
              3, 8, 5);
}

void fx_spawn_burst(int gx, int gy, int r, int g, int b, int count) {
    int n;
    for (n = 0; n < count; n++)
        spawn(gx, gy, r, g, b, 5, 10, 7);
}

void fx_shake(int frames) {
    shake_timer = frames;
}

void fx_update(void) {
    int i;
    for (i = 0; i < PART_MAX; i++) {
        if (parts[i].life > 0) {
            parts[i].x += parts[i].vx;
            parts[i].y += parts[i].vy;
            parts[i].life--;
        }
    }
    if (shake_timer > 0) {
        scr_set_offset(rng_range(3) - 1, rng_range(3) - 1);
        shake_timer--;
        if (shake_timer == 0)
            scr_set_offset(0, 0);
    }
}

void fx_draw_trail(void) {
    int i, r, g, b;
    for (i = 0; i < TRAIL_LEN; i++) {
        if (trail_gx[i] < 0) continue;
        r = 14 + (TRAIL_LEN - i) * 3;
        g = 15 + (TRAIL_LEN - i) * 16;
        b = 22 + (TRAIL_LEN - i) * 5;
        scr_cell(trail_gx[i], trail_gy[i], r, g, b);
    }
}

void fx_draw_parts(void) {
    int i, gx, gy, fade;
    for (i = 0; i < PART_MAX; i++) {
        if (parts[i].life > 0) {
            gx = parts[i].x >> 4;
            gy = parts[i].y >> 4;
            fade = (int)parts[i].life * 255 / (int)parts[i].max_life;
            scr_cell(gx, gy,
                     (int)parts[i].r * fade / 255,
                     (int)parts[i].g * fade / 255,
                     (int)parts[i].b * fade / 255);
        }
    }
}
