#ifndef ENTITY_H
#define ENTITY_H

#define ENT_MAX     1024
#define ENT_NONE    0xFF

#define ENT_SNAKE_HEAD  1
#define ENT_SNAKE_BODY  2
#define ENT_FOOD        3

#define ENT_ALIVE   (1 << 0)
#define ENT_VISIBLE (1 << 1)
#define ENT_AI      (1 << 2)

struct Entity {
    unsigned char x, y;
    unsigned char type;
    unsigned char flags;
    unsigned char chain_next;
    unsigned char chain_prev;
    unsigned char color_r, color_g, color_b;
    unsigned char timer;
    unsigned char owner;
    unsigned char pad;
};

void ent_init(int grid_w, int grid_h);
int  ent_alloc(void);
void ent_free(int id);
struct Entity *ent_get(int id);

void grid_set(int x, int y, unsigned char id);
void grid_clear(int x, int y);
int  grid_at(int x, int y);
int  grid_occupied(int x, int y);

#endif
