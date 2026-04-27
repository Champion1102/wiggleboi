#include "texture.h"
#include "screen.h"

unsigned char tex_data[TEX_COUNT * TEX_SIZE * TEX_SIZE * 3];
static unsigned char spr_data[SPR_COUNT * SPR_SIZE * SPR_SIZE * 4];

static int tex_hash(int x, int y, int seed) {
    int h = x * 374761393 + y * 668265263 + seed;
    h = (h ^ (h >> 13)) * 1274126177;
    h = h ^ (h >> 16);
    return h;
}

static int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }
static int iabs(int x) { return x < 0 ? -x : x; }

static void tex_set(int id, int tx, int ty, int r, int g, int b) {
    int off = (id * TEX_SIZE * TEX_SIZE + ty * TEX_SIZE + tx) * 3;
    tex_data[off]     = (unsigned char)clamp255(r);
    tex_data[off + 1] = (unsigned char)clamp255(g);
    tex_data[off + 2] = (unsigned char)clamp255(b);
}

static void spr_set(int id, int sx, int sy, int r, int g, int b, int opaque) {
    int off = (id * SPR_SIZE * SPR_SIZE + sy * SPR_SIZE + sx) * 4;
    spr_data[off]     = (unsigned char)clamp255(r);
    spr_data[off + 1] = (unsigned char)clamp255(g);
    spr_data[off + 2] = (unsigned char)clamp255(b);
    spr_data[off + 3] = (unsigned char)opaque;
}

/* ---- procedural wall textures ---- */

static void gen_brick(void) {
    int tx, ty;
    for (ty = 0; ty < TEX_SIZE; ty++) {
        for (tx = 0; tx < TEX_SIZE; tx++) {
            int row = ty / 8;
            int atx = (row & 1) ? ((tx + 8) & TEX_MASK) : tx;
            int mortar = (ty % 8 == 0) || (atx % 16 == 0);

            if (mortar) {
                int n = (tex_hash(tx, ty, 100) & 15) - 8;
                tex_set(TEX_BRICK, tx, ty, 45 + n, 38 + n, 32 + n);
            } else {
                int brick_id = (row * 4 + atx / 16);
                int n = (tex_hash(brick_id, brick_id * 7, 200) & 31) - 16;
                int spot = (tex_hash(tx, ty, 300) & 15) - 8;
                tex_set(TEX_BRICK, tx, ty,
                        140 + n + spot, 75 + n / 2 + spot / 2, 50 + n / 3 + spot / 3);
            }
        }
    }
}

static void gen_stone(void) {
    int tx, ty;
    for (ty = 0; ty < TEX_SIZE; ty++) {
        for (tx = 0; tx < TEX_SIZE; tx++) {
            int bx = tx / 16;
            int by = ty / 16;
            int block_id = bx + by * 4 + ((by & 1) ? 17 : 0);
            int n = (tex_hash(block_id, block_id * 3, 400) & 31) - 16;
            int spot = (tex_hash(tx, ty, 500) & 7) - 4;

            int edge_x = (tx % 16 == 0) || (tx % 16 == 15);
            int edge_y = (ty % 16 == 0) || (ty % 16 == 15);

            if (edge_x || edge_y) {
                int cn = (tex_hash(tx, ty, 450) & 7) - 4;
                tex_set(TEX_STONE, tx, ty, 70 + cn, 68 + cn, 62 + cn);
            } else {
                tex_set(TEX_STONE, tx, ty,
                        120 + n + spot, 115 + n + spot, 105 + n + spot);
            }
        }
    }
}

static void gen_metal(void) {
    int tx, ty;
    for (ty = 0; ty < TEX_SIZE; ty++) {
        for (tx = 0; tx < TEX_SIZE; tx++) {
            int panel_y = ty / 16;
            int in_y = ty % 16;
            int grad = 10 - (in_y * 12) / 16;
            int n = (tex_hash(tx, ty, 600) & 7) - 4;

            int rivet_line = (in_y == 1 || in_y == 14);
            int rivet = rivet_line && ((tx & 7) == 3);

            if (rivet) {
                tex_set(TEX_METAL, tx, ty, 150 + n, 155 + n, 170 + n);
            } else if (rivet_line) {
                tex_set(TEX_METAL, tx, ty, 50 + n, 55 + n, 68 + n);
            } else {
                int base_n = (tex_hash(panel_y, panel_y * 5, 650) & 15) - 8;
                tex_set(TEX_METAL, tx, ty,
                        60 + grad + n + base_n,
                        65 + grad + n + base_n,
                        80 + grad + n + base_n);
            }
        }
    }
}

static void gen_snake_tex(void) {
    int tx, ty;
    for (ty = 0; ty < TEX_SIZE; ty++) {
        for (tx = 0; tx < TEX_SIZE; tx++) {
            int dx = ((tx + ty) % 8);
            int dy = ((tx - ty + 64) % 8);
            int diamond = (dx < 4) ^ (dy < 4);
            int highlight = (dx == 1 && dy == 1);
            int n = (tex_hash(tx, ty, 700) & 7) - 4;

            if (highlight) {
                tex_set(TEX_SNAKE, tx, ty, 90 + n, 220 + n, 120 + n);
            } else if (diamond) {
                tex_set(TEX_SNAKE, tx, ty, 50 + n, 130 + n, 70 + n);
            } else {
                tex_set(TEX_SNAKE, tx, ty, 70 + n, 170 + n, 90 + n);
            }
        }
    }
}

static void gen_floor1(void) {
    int tx, ty;
    for (ty = 0; ty < TEX_SIZE; ty++) {
        for (tx = 0; tx < TEX_SIZE; tx++) {
            int bx = tx / 21;
            int by = ty / 21;
            int edge_x = (tx % 21 == 0);
            int edge_y = (ty % 21 == 0);
            int n = (tex_hash(tx, ty, 800) & 11) - 6;
            int block_n = (tex_hash(bx, by, 850) & 15) - 8;

            if (edge_x || edge_y) {
                tex_set(TEX_FLOOR_1, tx, ty, 22 + n, 20 + n, 16 + n);
            } else {
                tex_set(TEX_FLOOR_1, tx, ty,
                        38 + block_n + n, 35 + block_n + n, 30 + block_n + n);
            }
        }
    }
}

static void gen_floor2(void) {
    int tx, ty;
    for (ty = 0; ty < TEX_SIZE; ty++) {
        for (tx = 0; tx < TEX_SIZE; tx++) {
            int edge = (tx % 16 == 0) || (ty % 16 == 0);
            int n = (tex_hash(tx, ty, 900) & 5) - 3;

            if (edge) {
                tex_set(TEX_FLOOR_2, tx, ty, 30 + n, 28 + n, 26 + n);
            } else {
                tex_set(TEX_FLOOR_2, tx, ty, 20 + n, 18 + n, 16 + n);
            }
        }
    }
}

static void gen_ceil(void) {
    int tx, ty;
    for (ty = 0; ty < TEX_SIZE; ty++) {
        for (tx = 0; tx < TEX_SIZE; tx++) {
            int plank = ty / 8;
            int seam = (ty % 8 == 0);
            int n = (tex_hash(tx, ty, 1000) & 7) - 4;
            int plank_n = (tex_hash(plank, plank * 11, 1050) & 11) - 6;

            int knot = ((tex_hash(tx, plank, 1100) & 0xFF) < 4);

            if (seam) {
                tex_set(TEX_CEIL, tx, ty, 12 + n, 10 + n, 8 + n);
            } else if (knot) {
                tex_set(TEX_CEIL, tx, ty, 28 + n, 22 + n, 16 + n);
            } else {
                tex_set(TEX_CEIL, tx, ty,
                        20 + plank_n + n, 17 + plank_n + n, 14 + plank_n + n);
            }
        }
    }
}

static void gen_boundary(void) {
    int tx, ty;
    for (ty = 0; ty < TEX_SIZE; ty++) {
        for (tx = 0; tx < TEX_SIZE; tx++) {
            int stripe = ((tx + ty) / 8) & 1;
            int n = (tex_hash(tx, ty, 1200) & 7) - 4;

            if (stripe) {
                tex_set(TEX_BOUNDARY, tx, ty, 60 + n, 70 + n, 140 + n);
            } else {
                tex_set(TEX_BOUNDARY, tx, ty, 100 + n, 110 + n, 190 + n);
            }
        }
    }
}

/* ---- procedural sprites ---- */

static void gen_spr_food(void) {
    int sx, sy;
    for (sy = 0; sy < SPR_SIZE; sy++) {
        for (sx = 0; sx < SPR_SIZE; sx++) {
            int dx = sx - 7, dy = sy - 8;
            int dist2 = dx * dx + dy * dy;

            if (sy <= 2 && sx >= 7 && sx <= 9 && sy >= 0) {
                /* stem */
                if (sx == 8 && sy <= 2)
                    spr_set(SPR_FOOD, sx, sy, 80, 50, 30, 1);
                else if (sy <= 1 && sx >= 8 && sx <= 9)
                    spr_set(SPR_FOOD, sx, sy, 50, 160, 50, 1);
                else
                    spr_set(SPR_FOOD, sx, sy, 0, 0, 0, 0);
            } else if (dist2 < 36) {
                /* apple body */
                int shade = 200 - (dy + 3) * 12;
                int hi = (dx == -2 && dy == -2) ? 60 : 0;
                spr_set(SPR_FOOD, sx, sy,
                        clamp255(shade + hi), clamp255(30 + hi), clamp255(25 + hi / 2), 1);
            } else {
                spr_set(SPR_FOOD, sx, sy, 0, 0, 0, 0);
            }
        }
    }
}

static void gen_spr_bonus(void) {
    int sx, sy;
    for (sy = 0; sy < SPR_SIZE; sy++) {
        for (sx = 0; sx < SPR_SIZE; sx++) {
            int dx = sx - 7, dy = sy - 7;
            int dist2 = dx * dx + dy * dy;
            /* star: a circle with angular modulation */
            int angle5 = iabs(((dx * 181 + 128) >> 8) + ((dy * 181 + 128) >> 8)); /* rough */
            int star_r = 5 + ((angle5 % 5 < 2) ? 2 : -1);

            if (dist2 < star_r * star_r) {
                int shade = 220 - dist2 * 3;
                int hi = (dx == -1 && dy == -1) ? 35 : 0;
                spr_set(SPR_BONUS, sx, sy,
                        clamp255(shade + hi), clamp255(shade - 30 + hi), clamp255(40), 1);
            } else {
                spr_set(SPR_BONUS, sx, sy, 0, 0, 0, 0);
            }
        }
    }
}

static void gen_spr_poison(void) {
    int sx, sy;
    for (sy = 0; sy < SPR_SIZE; sy++) {
        for (sx = 0; sx < SPR_SIZE; sx++) {
            int dx = sx - 7, dy = sy - 7;
            int skull = 0, r = 0, g = 0, b = 0;

            /* skull outline: oval head */
            if (dy >= -5 && dy <= 2) {
                int hw = (dy < -2) ? 4 : (dy < 1 ? 5 : 3);
                if (iabs(dx) <= hw)
                    skull = 1;
            }
            /* jaw */
            if (dy >= 3 && dy <= 5 && iabs(dx) <= (5 - dy))
                skull = 1;

            /* eye sockets (holes in skull) */
            if (dy >= -3 && dy <= -1) {
                if ((dx >= -4 && dx <= -2) || (dx >= 2 && dx <= 4))
                    skull = 2;
            }
            /* nose */
            if (dy == 0 && iabs(dx) <= 1)
                skull = 2;

            if (skull == 1) {
                r = 160; g = 40; b = 180;
                int n = (tex_hash(sx, sy, 1500) & 15) - 8;
                spr_set(SPR_POISON, sx, sy, clamp255(r + n), clamp255(g + n / 2), clamp255(b + n), 1);
            } else if (skull == 2) {
                spr_set(SPR_POISON, sx, sy, 40, 10, 50, 1);
            } else {
                spr_set(SPR_POISON, sx, sy, 0, 0, 0, 0);
            }
        }
    }
}

static void gen_spr_powerup(void) {
    int sx, sy;
    /* lightning bolt shape via line segments */
    static const signed char bolt[][2] = {
        {8,1},{6,1},{7,1},  {5,2},{6,2},{7,2},
        {5,3},{6,3},        {4,4},{5,4},{6,4},
        {4,5},{5,5},        {3,6},{4,6},{5,6},
        {4,7},{5,7},{6,7},{7,7},{8,7},{9,7},
        {7,8},{8,8},{9,8},
        {8,9},{9,9},{10,9},
        {9,10},{10,10},     {10,11},{11,11},
        {10,12},{11,12},{12,12},
        {11,13},{12,13},
        {-1,-1}
    };
    int i;

    for (sy = 0; sy < SPR_SIZE; sy++)
        for (sx = 0; sx < SPR_SIZE; sx++)
            spr_set(SPR_POWERUP, sx, sy, 0, 0, 0, 0);

    for (i = 0; bolt[i][0] >= 0; i++) {
        int bx = bolt[i][0], by = bolt[i][1];
        int n = (tex_hash(bx, by, 1600) & 15);
        spr_set(SPR_POWERUP, bx, by,
                clamp255(60 + n), clamp255(200 + n), clamp255(255), 1);
    }
    /* bright core */
    spr_set(SPR_POWERUP, 6, 4, 180, 240, 255, 1);
    spr_set(SPR_POWERUP, 5, 5, 180, 240, 255, 1);
    spr_set(SPR_POWERUP, 8, 8, 180, 240, 255, 1);
    spr_set(SPR_POWERUP, 9, 10, 180, 240, 255, 1);
}

/* ---- weapon overlay (snake tongue) ---- */

/* 20x14 pixel art: 0=transparent, 1=dark green, 2=green, 3=bright green, 4=red tongue */
static const unsigned char weapon_art[14][20] = {
    {0,0,0,0,0,0,0,0,0,2,2,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,2,3,3,3,3,2,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,2,2,3,3,3,3,2,2,0,0,0,0,0,0},
    {0,0,0,0,0,1,2,3,3,3,3,3,3,2,1,0,0,0,0,0},
    {0,0,0,0,1,1,2,3,3,3,3,3,3,2,1,1,0,0,0,0},
    {0,0,0,1,1,2,2,3,3,3,3,3,3,2,2,1,1,0,0,0},
    {0,0,1,1,2,2,3,3,3,3,3,3,3,3,2,2,1,1,0,0},
    {0,1,1,2,2,2,3,3,3,3,3,3,3,3,2,2,2,1,1,0},
    {1,1,1,2,2,2,3,3,3,3,3,3,3,3,2,2,2,1,1,1},
    {0,0,0,0,0,0,0,0,4,0,0,4,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,4,0,0,0,0,4,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,4,0,0,0,0,0,0,4,0,0,0,0,0,0},
    {0,0,0,0,0,4,4,0,0,0,0,0,0,4,4,0,0,0,0,0},
};

void tex_init(void) {
    int i;
    for (i = 0; i < TEX_COUNT * TEX_SIZE * TEX_SIZE * 3; i++)
        tex_data[i] = 0;
    for (i = 0; i < SPR_COUNT * SPR_SIZE * SPR_SIZE * 4; i++)
        spr_data[i] = 0;

    gen_brick();
    gen_stone();
    gen_metal();
    gen_snake_tex();
    gen_floor1();
    gen_floor2();
    gen_ceil();
    gen_boundary();

    gen_spr_food();
    gen_spr_bonus();
    gen_spr_poison();
    gen_spr_powerup();
}

void spr_get(int id, int sx, int sy,
             unsigned char *r, unsigned char *g, unsigned char *b, int *transparent) {
    int off;
    if (id < 0 || id >= SPR_COUNT) { *transparent = 1; return; }
    sx &= SPR_MASK;
    sy &= SPR_MASK;
    off = (id * SPR_SIZE * SPR_SIZE + sy * SPR_SIZE + sx) * 4;
    *r = spr_data[off];
    *g = spr_data[off + 1];
    *b = spr_data[off + 2];
    *transparent = !spr_data[off + 3];
}

void tex_draw_weapon(int pw, int ph, int tick) {
    int tx, ty, px, py;
    int bob = ((tick >> 1) & 7);
    int base_x, base_y;
    unsigned char c;
    int r, g, b;

    bob = bob < 4 ? bob : 7 - bob;
    base_x = pw / 2 - 10;
    base_y = ph - 15 + bob;

    for (ty = 0; ty < 14; ty++) {
        for (tx = 0; tx < 20; tx++) {
            c = weapon_art[ty][tx];
            if (!c) continue;
            px = base_x + tx;
            py = base_y + ty;
            if (px < 0 || px >= pw || py < 0 || py >= ph) continue;

            switch (c) {
            case 1: r = 30;  g = 80;  b = 40;  break;
            case 2: r = 50;  g = 140; b = 70;  break;
            case 3: r = 80;  g = 200; b = 100; break;
            case 4: r = 200; g = 40;  b = 50;  break;
            default: continue;
            }
            scr_pixel(px, py, r, g, b);
        }
    }
}
