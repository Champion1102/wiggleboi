/*
 * entity.c — fixed-size entity pool + spatial grid.
 *
 * Entity pool: pre-allocated array, stack-based free list (OpenRCT2 pattern).
 * Spatial grid: O(1) collision detection per tile (replaces O(n) body scan).
 */

#include "entity.h"
#include <sys/mman.h>

static struct Entity pool[ENT_MAX];
static unsigned char free_stack[ENT_MAX];
static int free_top;
static unsigned char *grid;
static int gw, gh;
static void *grid_mem;

void ent_init(int grid_w, int grid_h) {
    int i;

    gw = grid_w;
    gh = grid_h;

    for (i = 0; i < ENT_MAX; i++) {
        pool[i].type = 0;
        pool[i].flags = 0;
        free_stack[i] = ENT_MAX - 1 - i;
    }
    free_top = ENT_MAX;

    grid_mem = mmap(0, (unsigned long)gw * gh, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANON, -1, 0);
    grid = (unsigned char *)grid_mem;

    for (i = 0; i < gw * gh; i++)
        grid[i] = ENT_NONE;
}

int ent_alloc(void) {
    int id;
    if (free_top <= 0) return -1;
    free_top--;
    id = free_stack[free_top];
    pool[id].flags = ENT_ALIVE;
    pool[id].chain_next = ENT_NONE;
    pool[id].chain_prev = ENT_NONE;
    return id;
}

void ent_free(int id) {
    if (id < 0 || id >= ENT_MAX) return;
    pool[id].type = 0;
    pool[id].flags = 0;
    free_stack[free_top] = id;
    free_top++;
}

struct Entity *ent_get(int id) {
    if (id < 0 || id >= ENT_MAX) return 0;
    return &pool[id];
}

void grid_set(int x, int y, unsigned char id) {
    if (x >= 0 && x < gw && y >= 0 && y < gh)
        grid[y * gw + x] = id;
}

void grid_clear(int x, int y) {
    if (x >= 0 && x < gw && y >= 0 && y < gh)
        grid[y * gw + x] = ENT_NONE;
}

int grid_at(int x, int y) {
    if (x < 0 || x >= gw || y < 0 || y >= gh) return -1;
    return grid[y * gw + x];
}

int grid_occupied(int x, int y) {
    if (x < 0 || x >= gw || y < 0 || y >= gh) return 1;
    return grid[y * gw + x] != ENT_NONE;
}
