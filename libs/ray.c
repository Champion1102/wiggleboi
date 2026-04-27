#include "ray.h"
#include "screen.h"
#include "entity.h"

#define FP_SHIFT  10
#define FP_ONE    (1 << FP_SHIFT)
#define FP_HALF   (FP_ONE >> 1)

#define ANGLE_360 1024
#define ANGLE_90  256
#define ANGLE_180 512

#define FOV       170
#define MAX_DEPTH 40
#define MAX_SPRITES 32

static int sw, sh;
static int gw, gh;

static int cam_x, cam_y;
static int cam_angle;
static int cam_head_id = -1;

static int sin_tab[ANGLE_360];
static int cos_tab[ANGLE_360];
static int depth_buf[512];
static int side_buf[512];
static int wtype_buf[512];

struct Sprite {
    int x, y;
    int dist;
    unsigned char r, g, b;
    unsigned char type;
};

static struct Sprite sprites[MAX_SPRITES];
static int num_sprites;

static int fp_mul(int a, int b) {
    return (int)(((long long)a * b) >> FP_SHIFT);
}

static int fp_div(int a, int b) {
    if (b == 0) return a > 0 ? 0x7FFFFFFF : -0x7FFFFFFF;
    return (int)(((long long)a << FP_SHIFT) / b);
}

static int iabs(int x) { return x < 0 ? -x : x; }

static void build_tables(void) {
    int i, val;
    long long num, den;

    for (i = 0; i < ANGLE_180; i++) {
        num = 16LL * i * (ANGLE_180 - i);
        den = 5LL * ANGLE_180 * ANGLE_180 - 4LL * i * (ANGLE_180 - i);
        if (den <= 0) den = 1;
        val = (int)(num * FP_ONE / den);
        if (val > FP_ONE) val = FP_ONE;
        sin_tab[i] = val;
    }
    for (i = ANGLE_180; i < ANGLE_360; i++)
        sin_tab[i] = -sin_tab[i - ANGLE_180];
    for (i = 0; i < ANGLE_360; i++)
        cos_tab[i] = sin_tab[(i + ANGLE_90) & (ANGLE_360 - 1)];
}

static int angle_wrap(int a) {
    return a & (ANGLE_360 - 1);
}

static int rc_sin(int a) { return sin_tab[angle_wrap(a)]; }
static int rc_cos(int a) { return cos_tab[angle_wrap(a)]; }

/* returns 0=empty, 1=boundary, 2=obstacle, 3=snake */
static int wall_at(int gx, int gy) {
    int id;
    struct Entity *e;

    if (gx < 0 || gx >= gw || gy < 0 || gy >= gh) return 1;
    if (!grid_occupied(gx, gy)) return 0;

    id = grid_at(gx, gy);
    if (id < 0) return 0;
    if (id == cam_head_id) return 0;

    e = ent_get(id);
    if (!e) return 0;
    if (e->type == ENT_OBSTACLE) return 2;
    if (e->type == ENT_SNAKE_BODY || e->type == ENT_SNAKE_HEAD) return 3;
    return 0;
}

void ray_init(int screen_w, int screen_h, int grid_w, int grid_h) {
    int i;
    sw = screen_w;
    sh = screen_h;
    gw = grid_w;
    gh = grid_h;
    cam_x = (gw / 2) * FP_ONE + FP_HALF;
    cam_y = (gh / 2) * FP_ONE + FP_HALF;
    cam_angle = 0;
    for (i = 0; i < 512; i++) {
        depth_buf[i] = MAX_DEPTH * FP_ONE;
        side_buf[i] = 0;
        wtype_buf[i] = 0;
    }
    build_tables();
}

void ray_set_camera(int pos_x, int pos_y, int angle) {
    cam_x = pos_x;
    cam_y = pos_y;
    cam_angle = angle_wrap(angle);
}

void ray_set_head(int hid) {
    cam_head_id = hid;
}

void ray_cast(void) {
    int col, ray_angle;
    int ray_dx, ray_dy;
    int map_x, map_y, step_x, step_y;
    int side_dist_x, side_dist_y, delta_dist_x, delta_dist_y;
    int side, depth, wt;

    for (col = 0; col < sw && col < 512; col++) {
        ray_angle = angle_wrap(cam_angle - FOV / 2 + (col * FOV) / sw);
        ray_dx = rc_cos(ray_angle);
        ray_dy = -rc_sin(ray_angle);

        map_x = cam_x >> FP_SHIFT;
        map_y = cam_y >> FP_SHIFT;

        delta_dist_x = ray_dx != 0 ? iabs(fp_div(FP_ONE, ray_dx)) : 0x7FFFFFFF;
        delta_dist_y = ray_dy != 0 ? iabs(fp_div(FP_ONE, ray_dy)) : 0x7FFFFFFF;

        if (ray_dx < 0) {
            step_x = -1;
            side_dist_x = fp_mul(cam_x - map_x * FP_ONE, delta_dist_x);
        } else {
            step_x = 1;
            side_dist_x = fp_mul((map_x + 1) * FP_ONE - cam_x, delta_dist_x);
        }
        if (ray_dy < 0) {
            step_y = -1;
            side_dist_y = fp_mul(cam_y - map_y * FP_ONE, delta_dist_y);
        } else {
            step_y = 1;
            side_dist_y = fp_mul((map_y + 1) * FP_ONE - cam_y, delta_dist_y);
        }

        wt = 0;
        side = 0;
        depth = 0;
        while (!wt && depth < MAX_DEPTH) {
            if (side_dist_x < side_dist_y) {
                map_x += step_x;
                side_dist_x += delta_dist_x;
                side = 0;
            } else {
                map_y += step_y;
                side_dist_y += delta_dist_y;
                side = 1;
            }
            depth++;
            wt = wall_at(map_x, map_y);
        }

        if (wt) {
            int perp;
            if (side == 0)
                perp = side_dist_x - delta_dist_x;
            else
                perp = side_dist_y - delta_dist_y;
            if (perp <= 0) perp = 1;

            int cos_diff = rc_cos(ray_angle - cam_angle);
            if (cos_diff <= 0) cos_diff = 1;
            perp = fp_mul(perp, cos_diff);
            if (perp <= 0) perp = 1;

            depth_buf[col] = perp;
            side_buf[col] = side;
            wtype_buf[col] = wt;
        } else {
            depth_buf[col] = MAX_DEPTH * FP_ONE;
            side_buf[col] = 0;
            wtype_buf[col] = 0;
        }
    }
}

void ray_draw_walls(void) {
    int col, wall_h, draw_start, draw_end, y;
    int perp, side, wt;
    int bright, fog, r, g, b;
    int base_r, base_g, base_b;
    int ray_angle, ray_dx, ray_dy;

    for (col = 0; col < sw; col++) {
        perp = depth_buf[col];
        side = side_buf[col];
        wt = wtype_buf[col];
        if (perp <= 0) perp = 1;

        wall_h = fp_div(sh * FP_ONE, perp) >> FP_SHIFT;
        if (wall_h > sh * 4) wall_h = sh * 4;
        draw_start = (sh - wall_h) / 2;
        draw_end = draw_start + wall_h;

        fog = perp / MAX_DEPTH;
        if (fog > FP_ONE) fog = FP_ONE;

        bright = FP_ONE - fog;
        if (side == 1) bright = bright * 7 / 10;
        if (bright < FP_ONE / 10) bright = FP_ONE / 10;

        switch (wt) {
        case 2:  base_r = 160; base_g = 130; base_b = 100; break;
        case 3:  base_r = 80;  base_g = 170; base_b = 100; break;
        default: base_r = 110; base_g = 120; base_b = 175; break;
        }

        r = (base_r * bright) >> FP_SHIFT;
        g = (base_g * bright) >> FP_SHIFT;
        b = (base_b * bright) >> FP_SHIFT;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;

        /* ceiling gradient */
        for (y = 0; y < draw_start && y < sh; y++) {
            int grad = y * 256 / (sh / 2 + 1);
            int cr = 5 + grad / 20;
            int cg = 5 + grad / 25;
            int cb = 15 + grad / 10;
            if (cr > 40) cr = 40;
            if (cg > 35) cg = 35;
            if (cb > 60) cb = 60;
            scr_pixel(col, y, cr, cg, cb);
        }

        /* wall strip */
        {
            int ws = draw_start < 0 ? 0 : draw_start;
            int we = draw_end > sh ? sh : draw_end;
            scr_vline(col, ws, we - 1, r, g, b);
        }

        /* floor with checkerboard */
        ray_angle = angle_wrap(cam_angle - FOV / 2 + (col * FOV) / sw);
        ray_dx = rc_cos(ray_angle);
        ray_dy = -rc_sin(ray_angle);

        {
            int cos_diff = rc_cos(ray_angle - cam_angle);
            if (cos_diff <= 0) cos_diff = 1;

            for (y = draw_end; y < sh; y++) {
                int dy_pix = y - sh / 2;
                int row_dist, fx, fy, checker;
                int fr, fg_, fb, ff;

                if (dy_pix <= 0) dy_pix = 1;
                row_dist = fp_div(sh / 2 * FP_ONE, dy_pix * cos_diff);
                if (row_dist < 0) row_dist = 0;

                fx = cam_x + fp_mul(row_dist, ray_dx);
                fy = cam_y - fp_mul(row_dist, ray_dy);
                checker = ((fx >> FP_SHIFT) ^ (fy >> FP_SHIFT)) & 1;

                ff = FP_ONE - row_dist / MAX_DEPTH;
                if (ff < FP_ONE / 8) ff = FP_ONE / 8;
                if (ff > FP_ONE) ff = FP_ONE;

                if (checker) {
                    fr = (32 * ff) >> FP_SHIFT;
                    fg_ = (40 * ff) >> FP_SHIFT;
                    fb = (28 * ff) >> FP_SHIFT;
                } else {
                    fr = (18 * ff) >> FP_SHIFT;
                    fg_ = (22 * ff) >> FP_SHIFT;
                    fb = (16 * ff) >> FP_SHIFT;
                }
                scr_pixel(col, y, fr, fg_, fb);
            }
        }
    }
}

static void collect_sprites(void) {
    int i, dx, dy;
    struct Entity *e;

    num_sprites = 0;
    for (i = 0; i < ENT_MAX && num_sprites < MAX_SPRITES; i++) {
        e = ent_get(i);
        if (!e || !(e->flags & ENT_ALIVE)) continue;
        if (e->type == ENT_SNAKE_HEAD || e->type == ENT_SNAKE_BODY) continue;
        if (e->type == ENT_OBSTACLE) continue;

        dx = (e->x * FP_ONE + FP_HALF) - cam_x;
        dy = (e->y * FP_ONE + FP_HALF) - cam_y;

        sprites[num_sprites].x = e->x * FP_ONE + FP_HALF;
        sprites[num_sprites].y = e->y * FP_ONE + FP_HALF;
        sprites[num_sprites].dist = fp_mul(dx, dx) + fp_mul(dy, dy);
        sprites[num_sprites].type = e->type;

        switch (e->type) {
        case ENT_FOOD:    sprites[num_sprites].r = 220; sprites[num_sprites].g = 50;  sprites[num_sprites].b = 30;  break;
        case ENT_BONUS:   sprites[num_sprites].r = 255; sprites[num_sprites].g = 220; sprites[num_sprites].b = 40;  break;
        case ENT_POISON:  sprites[num_sprites].r = 160; sprites[num_sprites].g = 30;  sprites[num_sprites].b = 180; break;
        case ENT_POWERUP: sprites[num_sprites].r = 60;  sprites[num_sprites].g = 180; sprites[num_sprites].b = 240; break;
        default:          sprites[num_sprites].r = 200; sprites[num_sprites].g = 200; sprites[num_sprites].b = 200; break;
        }
        num_sprites++;
    }
}

static void sort_sprites(void) {
    int i, j;
    struct Sprite tmp;
    for (i = 1; i < num_sprites; i++) {
        tmp = sprites[i];
        j = i - 1;
        while (j >= 0 && sprites[j].dist < tmp.dist) {
            sprites[j + 1] = sprites[j];
            j--;
        }
        sprites[j + 1] = tmp;
    }
}

void ray_draw_sprites(int tick) {
    int i, dx, dy;
    int tx, tz;
    int cos_a, sin_a;
    int spr_screen_x, spr_h, spr_w;
    int draw_sx, draw_ex;
    int stripe, y;
    int fog, bright;
    int pulse, pr, pg, pb, sr, sg, sb;
    int cx, dx_px, half_h, max_y_off, top, bot;

    collect_sprites();
    sort_sprites();

    cos_a = rc_cos(cam_angle);
    sin_a = rc_sin(cam_angle);

    for (i = 0; i < num_sprites; i++) {
        dx = sprites[i].x - cam_x;
        dy = sprites[i].y - cam_y;

        /* rotate into camera space: tz = depth, tx = lateral */
        tz = fp_mul(dx, cos_a) - fp_mul(dy, sin_a);
        tx = fp_mul(dx, sin_a) + fp_mul(dy, cos_a);

        if (tz <= FP_ONE / 4) continue;

        spr_screen_x = sw / 2 + (int)((long long)tx * sw / (2LL * tz));
        spr_h = iabs(fp_div(sh * FP_ONE, tz)) >> FP_SHIFT;
        if (spr_h > sh * 2) spr_h = sh * 2;
        spr_w = spr_h;

        draw_sx = spr_screen_x - spr_w / 2;
        draw_ex = spr_screen_x + spr_w / 2;
        if (draw_sx < 0) draw_sx = 0;
        if (draw_ex > sw) draw_ex = sw;

        fog = (tz * FP_ONE) / (MAX_DEPTH * FP_ONE);
        if (fog > FP_ONE) fog = FP_ONE;
        bright = FP_ONE - fog;
        if (bright < FP_ONE / 8) bright = FP_ONE / 8;

        pulse = ((tick >> 1) & 7);
        pulse = pulse < 4 ? pulse : 7 - pulse;
        pr = sprites[i].r + pulse * 4;
        pg = sprites[i].g + pulse * 3;
        pb = sprites[i].b + pulse * 4;
        if (pr > 255) pr = 255;
        if (pg > 255) pg = 255;
        if (pb > 255) pb = 255;

        sr = (pr * bright) >> FP_SHIFT;
        sg = (pg * bright) >> FP_SHIFT;
        sb = (pb * bright) >> FP_SHIFT;
        if (sr > 255) sr = 255;
        if (sg > 255) sg = 255;
        if (sb > 255) sb = 255;

        for (stripe = draw_sx; stripe < draw_ex; stripe++) {
            if (stripe >= 512 || tz >= depth_buf[stripe]) continue;

            cx = spr_screen_x;
            dx_px = iabs(stripe - cx);
            half_h = spr_h / 2;
            if (half_h <= 0) half_h = 1;
            max_y_off = half_h - (dx_px * half_h) / (spr_w / 2 + 1);
            if (max_y_off <= 0) continue;

            top = sh / 2 - max_y_off;
            bot = sh / 2 + max_y_off;
            if (top < 0) top = 0;
            if (bot > sh) bot = sh;

            for (y = top; y < bot; y++)
                scr_pixel(stripe, y, sr, sg, sb);
        }
    }
}

int ray_depth_at(int col) {
    if (col < 0 || col >= sw || col >= 512) return MAX_DEPTH * FP_ONE;
    return depth_buf[col];
}
