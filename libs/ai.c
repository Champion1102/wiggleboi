/*
 * ai.c -- AI opponent snakes with RCT-inspired bounded pathfinding.
 *
 * Each AI snake uses a depth-limited BFS (max 5) toward food,
 * with random wander fallback. Expensive pathfinding is distributed:
 * only 1 AI gets BFS per tick (OpenRCT2's index & mask == tick & mask).
 * Personalities: Aggressive (beelines food), Cautious (avoids player),
 * Random (pure wander).
 */

#include "ai.h"
#include "entity.h"
#include "screen.h"
#include "math.h"
#include "fx.h"

#define BFS_DEPTH 5
#define BFS_QUEUE 128
#define RESPAWN_TICKS 50

enum { PERS_AGGRESSIVE, PERS_CAUTIOUS, PERS_RANDOM };

struct AISnake {
    int head_id, tail_id;
    int len;
    int dx, dy;
    int alive;
    int respawn;
    int personality;
    unsigned char cr, cg, cb;
};

static struct AISnake ais[AI_MAX];
static int ai_w, ai_h;
static int ate_flag;

static const unsigned char ai_cols[3][3] = {
    {220, 80, 50},
    {90, 110, 230},
    {210, 200, 60}
};

static const int ai_thresh[AI_MAX] = {8, 16, 25, 35};

static const int dx4[4] = {0, 0, -1, 1};
static const int dy4[4] = {-1, 1, 0, 0};

static int can_pass(int x, int y) {
    int gv;
    struct Entity *e;

    if (x < 0 || x >= ai_w || y < 0 || y >= ai_h) return 0;
    gv = grid_at(x, y);
    if (gv == ENT_NONE) return 1;
    if (gv < 0) return 0;
    e = ent_get(gv);
    return e && e->type == ENT_FOOD;
}

/* ---- Chain helpers (mirror player snake logic) ---- */

static int ai_push_head(int idx, int x, int y) {
    struct AISnake *a = &ais[idx];
    int id = ent_alloc();
    struct Entity *e, *old;

    if (id < 0) return -1;
    e = ent_get(id);
    e->x = x;
    e->y = y;
    e->type = ENT_SNAKE_HEAD;
    e->flags = ENT_ALIVE | ENT_VISIBLE | ENT_AI;
    e->owner = idx;

    if (a->head_id >= 0) {
        old = ent_get(a->head_id);
        old->type = ENT_SNAKE_BODY;
        old->chain_next = id;
        e->chain_prev = a->head_id;
    } else {
        a->tail_id = id;
    }

    a->head_id = id;
    grid_set(x, y, id);
    a->len++;
    return id;
}

static void ai_pop_tail(int idx) {
    struct AISnake *a = &ais[idx];
    struct Entity *t;
    int next;

    if (a->tail_id < 0) return;
    t = ent_get(a->tail_id);
    grid_clear(t->x, t->y);
    next = t->chain_next;
    ent_free(a->tail_id);

    a->tail_id = next;
    if (a->tail_id >= 0 && a->tail_id != ENT_NONE) {
        ent_get(a->tail_id)->chain_prev = ENT_NONE;
    } else {
        a->tail_id = -1;
        a->head_id = -1;
    }
    a->len--;
}

static void ai_kill_one(int idx) {
    struct AISnake *a = &ais[idx];
    int id, next;
    struct Entity *e;

    if (!a->alive) return;

    id = a->tail_id;
    while (id >= 0 && id != ENT_NONE) {
        e = ent_get(id);
        next = e->chain_next;
        fx_spawn_burst(e->x, e->y, a->cr / 2, a->cg / 2, a->cb / 2, 2);
        grid_clear(e->x, e->y);
        ent_free(id);
        id = next;
    }

    a->head_id = -1;
    a->tail_id = -1;
    a->len = 0;
    a->alive = 0;
    a->respawn = RESPAWN_TICKS;
}

/* ---- Pathfinding ---- */

struct BNode {
    unsigned char x, y, first_dir, depth;
};

static int bfs_toward(int sx, int sy, int tx, int ty, int *odx, int *ody) {
    struct BNode q[BFS_QUEUE];
    unsigned char vis[2048];
    int front = 0, back = 0;
    int i, nx, ny, dist, best_dist, best_dir, cells;

    cells = ai_w * ai_h;
    for (i = 0; i < cells; i++) vis[i] = 0;
    vis[sy * ai_w + sx] = 1;

    best_dist = 9999;
    best_dir = -1;

    for (i = 0; i < 4; i++) {
        nx = sx + dx4[i];
        ny = sy + dy4[i];
        if (!can_pass(nx, ny) && !(nx == tx && ny == ty)) continue;
        if (nx < 0 || nx >= ai_w || ny < 0 || ny >= ai_h) continue;
        vis[ny * ai_w + nx] = 1;
        q[back].x = nx;
        q[back].y = ny;
        q[back].first_dir = i;
        q[back].depth = 1;
        back++;
    }

    while (front < back) {
        struct BNode n = q[front++];
        if (n.x == tx && n.y == ty) {
            *odx = dx4[n.first_dir];
            *ody = dy4[n.first_dir];
            return 1;
        }

        dist = (n.x > tx ? n.x - tx : tx - n.x) +
               (n.y > ty ? n.y - ty : ty - n.y);
        if (dist < best_dist) {
            best_dist = dist;
            best_dir = n.first_dir;
        }

        if (n.depth >= BFS_DEPTH) continue;

        for (i = 0; i < 4; i++) {
            nx = n.x + dx4[i];
            ny = n.y + dy4[i];
            if (nx < 0 || nx >= ai_w || ny < 0 || ny >= ai_h) continue;
            if (vis[ny * ai_w + nx]) continue;
            if (!can_pass(nx, ny) && !(nx == tx && ny == ty)) continue;
            if (back >= BFS_QUEUE) break;
            vis[ny * ai_w + nx] = 1;
            q[back].x = nx;
            q[back].y = ny;
            q[back].first_dir = n.first_dir;
            q[back].depth = n.depth + 1;
            back++;
        }
    }

    if (best_dir >= 0) {
        *odx = dx4[best_dir];
        *ody = dy4[best_dir];
        return 1;
    }
    return 0;
}

static int random_valid_dir(int x, int y, int cdx, int cdy, int *odx, int *ody) {
    int dirs[4], nd = 0;
    int i, nx, ny;

    for (i = 0; i < 4; i++) {
        if (dx4[i] == -cdx && dy4[i] == -cdy) continue;
        nx = x + dx4[i];
        ny = y + dy4[i];
        if (can_pass(nx, ny))
            dirs[nd++] = i;
    }

    if (nd == 0) {
        *odx = -cdx;
        *ody = -cdy;
        return 0;
    }

    i = dirs[rng_range(nd)];
    *odx = dx4[i];
    *ody = dy4[i];
    return 1;
}

/* ---- Public API ---- */

void ai_init(int w, int h) {
    int i;
    ai_w = w;
    ai_h = h;
    ate_flag = 0;
    for (i = 0; i < AI_MAX; i++) {
        ais[i].alive = 0;
        ais[i].head_id = -1;
        ais[i].tail_id = -1;
        ais[i].len = 0;
        ais[i].respawn = 0;
    }
}

void ai_kill_all(void) {
    int i;
    for (i = 0; i < AI_MAX; i++)
        ai_kill_one(i);
}

void ai_reset(void) {
    ai_kill_all();
    ai_init(ai_w, ai_h);
}

int ai_spawn(int idx) {
    struct AISnake *a;
    int x, y, i, d, ddx, ddy, ok, att;

    if (idx < 0 || idx >= AI_MAX) return 0;
    a = &ais[idx];
    if (a->alive) return 0;

    att = 50;
    while (att-- > 0) {
        x = rng_range(ai_w - 8) + 4;
        y = rng_range(ai_h - 6) + 3;
        d = rng_range(4);
        ddx = dx4[d];
        ddy = dy4[d];

        ok = 1;
        for (i = 0; i < 3; i++) {
            int cx = x - ddx * i;
            int cy = y - ddy * i;
            if (cx < 0 || cx >= ai_w || cy < 0 || cy >= ai_h ||
                grid_occupied(cx, cy)) {
                ok = 0;
                break;
            }
        }
        if (!ok) continue;

        a->dx = ddx;
        a->dy = ddy;
        a->alive = 1;
        a->respawn = 0;
        a->personality = idx % 3;
        a->cr = ai_cols[a->personality][0];
        a->cg = ai_cols[a->personality][1];
        a->cb = ai_cols[a->personality][2];
        a->head_id = -1;
        a->tail_id = -1;
        a->len = 0;

        for (i = 2; i >= 0; i--)
            ai_push_head(idx, x - ddx * i, y - ddy * i);

        return 1;
    }
    return 0;
}

void ai_update(int tick, int food_x, int food_y, int px, int py, int cur_score) {
    int i, nx, ny, gval, ndx, ndy, mdist;
    struct AISnake *a;
    struct Entity *h, *ge;

    ate_flag = 0;

    for (i = 0; i < AI_MAX; i++) {
        a = &ais[i];

        if (!a->alive) {
            if (a->respawn > 0)
                a->respawn--;
            else if (cur_score >= ai_thresh[i])
                ai_spawn(i);
            continue;
        }

        h = ent_get(a->head_id);
        ndx = a->dx;
        ndy = a->dy;

        /* Distributed pathfinding: 1 AI per tick gets BFS */
        if ((i & 3) == (tick & 3)) {
            switch (a->personality) {
            case PERS_AGGRESSIVE:
                if (!bfs_toward(h->x, h->y, food_x, food_y, &ndx, &ndy))
                    random_valid_dir(h->x, h->y, a->dx, a->dy, &ndx, &ndy);
                break;

            case PERS_CAUTIOUS:
                mdist = (h->x > px ? h->x - px : px - h->x) +
                        (h->y > py ? h->y - py : py - h->y);
                if (mdist < 6) {
                    ndx = h->x >= px ? 1 : -1;
                    ndy = 0;
                    if (!can_pass(h->x + ndx, h->y)) {
                        ndx = 0;
                        ndy = h->y >= py ? 1 : -1;
                    }
                    if (!can_pass(h->x + ndx, h->y + ndy))
                        random_valid_dir(h->x, h->y, a->dx, a->dy, &ndx, &ndy);
                } else {
                    if (!bfs_toward(h->x, h->y, food_x, food_y, &ndx, &ndy))
                        random_valid_dir(h->x, h->y, a->dx, a->dy, &ndx, &ndy);
                }
                break;

            case PERS_RANDOM:
                if (rng_range(3) == 0)
                    random_valid_dir(h->x, h->y, a->dx, a->dy, &ndx, &ndy);
                break;
            }
        }

        a->dx = ndx;
        a->dy = ndy;

        /* Obstacle avoidance every tick */
        nx = h->x + a->dx;
        ny = h->y + a->dy;

        if (!can_pass(nx, ny)) {
            gval = grid_at(nx, ny);
            ge = (gval >= 0 && gval != ENT_NONE) ? ent_get(gval) : 0;
            if (ge && ge->type == ENT_FOOD) {
                goto do_move;
            }
            if (!random_valid_dir(h->x, h->y, a->dx, a->dy, &ndx, &ndy)) {
                ai_kill_one(i);
                continue;
            }
            a->dx = ndx;
            a->dy = ndy;
            nx = h->x + a->dx;
            ny = h->y + a->dy;
        }

do_move:
        if (nx < 0 || nx >= ai_w || ny < 0 || ny >= ai_h) {
            ai_kill_one(i);
            continue;
        }

        gval = grid_at(nx, ny);

        if (gval == ENT_NONE) {
            ai_pop_tail(i);
            ai_push_head(i, nx, ny);
        } else {
            ge = ent_get(gval);
            if (ge && ge->type == ENT_FOOD) {
                ai_push_head(i, nx, ny);
                ate_flag = 1;
            } else {
                ai_kill_one(i);
            }
        }
    }
}

void ai_draw(int tick) {
    int i, id, j, maxd, dist, r, g, b;
    struct AISnake *a;
    struct Entity *e;

    (void)tick;

    for (i = 0; i < AI_MAX; i++) {
        a = &ais[i];
        if (!a->alive) continue;

        maxd = a->len > 1 ? a->len - 1 : 1;
        id = a->tail_id;
        j = 0;

        while (id >= 0 && id != ENT_NONE) {
            e = ent_get(id);
            if (!(e->flags & ENT_ALIVE)) {
                id = e->chain_next;
                j++;
                continue;
            }

            if (e->type == ENT_SNAKE_HEAD) {
                scr_cell(e->x, e->y, a->cr, a->cg, a->cb);
            } else {
                dist = a->len - 1 - j;
                r = a->cr - (dist * (a->cr / 3)) / maxd;
                g = a->cg - (dist * (a->cg / 3)) / maxd;
                b = a->cb - (dist * (a->cb / 3)) / maxd;
                if (r < 15) r = 15;
                if (g < 15) g = 15;
                if (b < 15) b = 15;
                scr_cell(e->x, e->y, r, g, b);
            }
            id = e->chain_next;
            j++;
        }
    }
}

int ai_ate_food(void) {
    return ate_flag;
}

int ai_alive(int idx) {
    if (idx < 0 || idx >= AI_MAX) return 0;
    return ais[idx].alive;
}
