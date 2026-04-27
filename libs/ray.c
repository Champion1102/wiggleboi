#include "ray.h"
#include "screen.h"
#include "entity.h"
#include "texture.h"

#define FP_SHIFT  10
#define FP_ONE    (1 << FP_SHIFT)
#define FP_HALF   (FP_ONE >> 1)

#define ANGLE_360 1024
#define ANGLE_90  256
#define ANGLE_180 512

#define FOV       170
#define MAX_DEPTH 40
#define MAX_SPRITES 32

#define FOG_R 8
#define FOG_G 10
#define FOG_B 20

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
static int wall_tex_x[512];

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
        wall_tex_x[i] = 0;
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
            int perp, wall_x;
            if (side == 0)
                perp = side_dist_x - delta_dist_x;
            else
                perp = side_dist_y - delta_dist_y;
            if (perp <= 0) perp = 1;

            /* compute texture X from wall hit position */
            if (side == 0)
                wall_x = cam_y + fp_mul(perp, ray_dy);
            else
                wall_x = cam_x + fp_mul(perp, ray_dx);
            wall_x = wall_x & (FP_ONE - 1);
            wall_tex_x[col] = (wall_x * TEX_SIZE) >> FP_SHIFT;
            wall_tex_x[col] &= TEX_MASK;

            /* fisheye correction */
            {
                int cos_diff = rc_cos(ray_angle - cam_angle);
                if (cos_diff <= 0) cos_diff = 1;
                perp = fp_mul(perp, cos_diff);
                if (perp <= 0) perp = 1;
            }

            depth_buf[col] = perp;
            side_buf[col] = side;
            wtype_buf[col] = wt;
        } else {
            depth_buf[col] = MAX_DEPTH * FP_ONE;
            side_buf[col] = 0;
            wtype_buf[col] = 0;
            wall_tex_x[col] = 0;
        }
    }
}

static int apply_fog(int channel, int fog_ch, int fog_factor) {
    return ((channel * (FP_ONE - fog_factor)) + (fog_ch * fog_factor)) >> FP_SHIFT;
}

static int compute_wall_light(int perp, int side, int wt, int y,
                              int wall_h, int draw_start, int tick) {
    int norm, dist_atten, side_dim, vert_grad, flicker, light;

    norm = (perp * 256) / (MAX_DEPTH * FP_ONE);
    if (norm > 256) norm = 256;
    dist_atten = 256 - (norm * norm) / 256;
    if (dist_atten < 30) dist_atten = 30;

    side_dim = side ? 180 : 256;

    if (wall_h > 0) {
        int vy = y - draw_start;
        vert_grad = 256 - (vy * 25) / wall_h;
    } else {
        vert_grad = 256;
    }

    flicker = 256;
    if (wt == 2) {
        int f = ((tick * 7 + perp) >> 3) & 15;
        flicker = 242 + f;
        if (flicker > 256) flicker = 256;
    }

    light = (dist_atten * side_dim) >> 8;
    light = (light * vert_grad) >> 8;
    light = (light * flicker) >> 8;
    if (light > 256) light = 256;
    if (light < 18) light = 18;
    return light;
}

void ray_draw_walls(int tick) {
    int col, wall_h, draw_start, draw_end, y;
    int perp, side, wt;
    int ray_angle, ray_dx, ray_dy, cos_diff;

    for (col = 0; col < sw; col++) {
        perp = depth_buf[col];
        side = side_buf[col];
        wt = wtype_buf[col];
        if (perp <= 0) perp = 1;

        wall_h = fp_div(sh * FP_ONE, perp) >> FP_SHIFT;
        if (wall_h > sh * 4) wall_h = sh * 4;
        draw_start = (sh - wall_h) / 2;
        draw_end = draw_start + wall_h;

        ray_angle = angle_wrap(cam_angle - FOV / 2 + (col * FOV) / sw);
        ray_dx = rc_cos(ray_angle);
        ray_dy = -rc_sin(ray_angle);
        cos_diff = rc_cos(ray_angle - cam_angle);
        if (cos_diff <= 0) cos_diff = 1;

        /* ---- ceiling: textured near, sky far ---- */
        for (y = 0; y < draw_start && y < sh; y++) {
            int dy_pix = sh / 2 - y;
            if (dy_pix <= 0) dy_pix = 1;

            int row_dist = fp_div(sh / 2 * FP_ONE, dy_pix * cos_diff);
            if (row_dist < 0) row_dist = 0;

            if (row_dist > MAX_DEPTH * FP_ONE / 2) {
                /* sky gradient + stars */
                int sky_t = y * 256 / (sh / 2 + 1);
                int sr = 5 + sky_t / 15;
                int sg = 5 + sky_t / 20;
                int sb = 18 + sky_t / 4;

                int star_seed = (col * 7919 + y * 104729 +
                                 (cam_angle >> 2) * 31) & 0xFFFF;
                if ((star_seed & 0x3FF) < 3) {
                    sr = 180 + ((star_seed >> 12) & 3) * 18;
                    sg = 180 + ((star_seed >> 10) & 3) * 15;
                    sb = 200 + ((star_seed >> 8) & 3) * 12;
                }
                scr_pixel(col, y, sr, sg, sb);
            } else {
                int cx = cam_x + fp_mul(row_dist, ray_dx);
                int cy = cam_y - fp_mul(row_dist, ray_dy);
                int ctx = ((cx & (FP_ONE - 1)) * TEX_SIZE) >> FP_SHIFT;
                int cty = ((cy & (FP_ONE - 1)) * TEX_SIZE) >> FP_SHIFT;
                unsigned char tr, tg, tb;
                int cf;

                ctx &= TEX_MASK;
                cty &= TEX_MASK;
                tex_get(TEX_CEIL, ctx, cty, &tr, &tg, &tb);

                cf = FP_ONE - row_dist * 4 / (MAX_DEPTH * 3);
                if (cf < FP_ONE / 12) cf = FP_ONE / 12;
                if (cf > FP_ONE) cf = FP_ONE;

                scr_pixel(col, y,
                          apply_fog(((int)tr * cf) >> FP_SHIFT, FOG_R, FP_ONE - cf),
                          apply_fog(((int)tg * cf) >> FP_SHIFT, FOG_G, FP_ONE - cf),
                          apply_fog(((int)tb * cf) >> FP_SHIFT, FOG_B, FP_ONE - cf));
            }
        }

        /* ---- wall strip: textured ---- */
        {
            int ws = draw_start < 0 ? 0 : draw_start;
            int we = draw_end > sh ? sh : draw_end;
            int tex_id, tex_x_col;

            switch (wt) {
            case 1:  tex_id = TEX_BOUNDARY; break;
            case 2:  tex_id = TEX_STONE;    break;
            case 3:  tex_id = TEX_SNAKE;    break;
            default: tex_id = TEX_BRICK;    break;
            }
            tex_x_col = wall_tex_x[col];

            /* animate snake texture */
            if (tex_id == TEX_SNAKE) {
                tex_x_col = (tex_x_col + (tick >> 2)) & TEX_MASK;
            }

            for (y = ws; y < we; y++) {
                int tex_y = ((y - draw_start) * TEX_SIZE) / wall_h;
                unsigned char tr, tg, tb;
                int light, pr, pg, pb, fog_f;

                tex_y &= TEX_MASK;
                tex_get(tex_id, tex_x_col, tex_y, &tr, &tg, &tb);

                light = compute_wall_light(perp, side, wt, y,
                                           wall_h, draw_start, tick);
                pr = ((int)tr * light) >> 8;
                pg = ((int)tg * light) >> 8;
                pb = ((int)tb * light) >> 8;

                /* colored fog blend */
                fog_f = perp * FP_ONE / (MAX_DEPTH * FP_ONE);
                if (fog_f > FP_ONE) fog_f = FP_ONE;
                if (fog_f < 0) fog_f = 0;
                pr = apply_fog(pr, FOG_R, fog_f);
                pg = apply_fog(pg, FOG_G, fog_f);
                pb = apply_fog(pb, FOG_B, fog_f);

                if (pr > 255) pr = 255;
                if (pg > 255) pg = 255;
                if (pb > 255) pb = 255;
                scr_pixel(col, y, pr, pg, pb);
            }
        }

        /* ---- floor: textured ---- */
        for (y = draw_end; y < sh; y++) {
            int dy_pix = y - sh / 2;
            int row_dist, fx, fy;
            int ftx, fty, tile_type, floor_tex;
            unsigned char tr, tg, tb;
            int ff, fr, fg_, fb;

            if (dy_pix <= 0) dy_pix = 1;
            row_dist = fp_div(sh / 2 * FP_ONE, dy_pix * cos_diff);
            if (row_dist < 0) row_dist = 0;

            fx = cam_x + fp_mul(row_dist, ray_dx);
            fy = cam_y - fp_mul(row_dist, ray_dy);

            ftx = ((fx & (FP_ONE - 1)) * TEX_SIZE) >> FP_SHIFT;
            fty = ((fy & (FP_ONE - 1)) * TEX_SIZE) >> FP_SHIFT;
            ftx &= TEX_MASK;
            fty &= TEX_MASK;

            tile_type = ((fx >> FP_SHIFT) ^ (fy >> FP_SHIFT)) & 1;
            floor_tex = tile_type ? TEX_FLOOR_1 : TEX_FLOOR_2;
            tex_get(floor_tex, ftx, fty, &tr, &tg, &tb);

            ff = FP_ONE - row_dist / MAX_DEPTH;
            if (ff < FP_ONE / 8) ff = FP_ONE / 8;
            if (ff > FP_ONE) ff = FP_ONE;

            fr = ((int)tr * ff) >> FP_SHIFT;
            fg_ = ((int)tg * ff) >> FP_SHIFT;
            fb = ((int)tb * ff) >> FP_SHIFT;

            /* colored fog */
            {
                int fog_f = FP_ONE - ff;
                fr = apply_fog(fr, FOG_R, fog_f);
                fg_ = apply_fog(fg_, FOG_G, fog_f);
                fb = apply_fog(fb, FOG_B, fog_f);
            }

            scr_pixel(col, y, fr, fg_, fb);
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
    int pulse;

    collect_sprites();
    sort_sprites();

    cos_a = rc_cos(cam_angle);
    sin_a = rc_sin(cam_angle);

    for (i = 0; i < num_sprites; i++) {
        int spr_id;
        dx = sprites[i].x - cam_x;
        dy = sprites[i].y - cam_y;

        tz = fp_mul(dx, cos_a) - fp_mul(dy, sin_a);
        tx = fp_mul(dx, sin_a) + fp_mul(dy, cos_a);

        if (tz <= FP_ONE / 4) continue;

        spr_screen_x = sw / 2 + (int)((long long)tx * sw / (2LL * tz));
        spr_h = iabs(fp_div(sh * FP_ONE, tz)) >> FP_SHIFT;
        if (spr_h > sh * 2) spr_h = sh * 2;
        if (spr_h < 1) spr_h = 1;
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

        switch (sprites[i].type) {
        case ENT_FOOD:    spr_id = SPR_FOOD;    break;
        case ENT_BONUS:   spr_id = SPR_BONUS;   break;
        case ENT_POISON:  spr_id = SPR_POISON;  break;
        case ENT_POWERUP: spr_id = SPR_POWERUP; break;
        default:          spr_id = SPR_FOOD;     break;
        }

        for (stripe = draw_sx; stripe < draw_ex; stripe++) {
            int top, bot, stx;
            if (stripe >= 512 || tz >= depth_buf[stripe]) continue;

            stx = ((stripe - (spr_screen_x - spr_w / 2)) * SPR_SIZE) / spr_w;
            if (stx < 0) stx = 0;
            if (stx >= SPR_SIZE) stx = SPR_SIZE - 1;

            top = sh / 2 - spr_h / 2;
            bot = sh / 2 + spr_h / 2;
            if (top < 0) top = 0;
            if (bot > sh) bot = sh;

            for (y = top; y < bot; y++) {
                unsigned char sr, sg, sb;
                int transparent, sty;
                int lr, lg, lb;

                sty = ((y - (sh / 2 - spr_h / 2)) * SPR_SIZE) / spr_h;
                if (sty < 0) sty = 0;
                if (sty >= SPR_SIZE) sty = SPR_SIZE - 1;

                spr_get(spr_id, stx, sty, &sr, &sg, &sb, &transparent);
                if (transparent) continue;

                lr = ((int)sr * bright) >> FP_SHIFT;
                lg = ((int)sg * bright) >> FP_SHIFT;
                lb = ((int)sb * bright) >> FP_SHIFT;

                lr += pulse * 4; if (lr > 255) lr = 255;
                lg += pulse * 3; if (lg > 255) lg = 255;
                lb += pulse * 4; if (lb > 255) lb = 255;

                scr_pixel(stripe, y, lr, lg, lb);
            }
        }
    }
}

int ray_depth_at(int col) {
    if (col < 0 || col >= sw || col >= 512) return MAX_DEPTH * FP_ONE;
    return depth_buf[col];
}
