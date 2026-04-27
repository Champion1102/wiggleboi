#ifndef TEXTURE_H
#define TEXTURE_H

#define TEX_SIZE   64
#define TEX_SHIFT   6
#define TEX_MASK   63

#define TEX_BRICK     0
#define TEX_STONE     1
#define TEX_METAL     2
#define TEX_SNAKE     3
#define TEX_FLOOR_1   4
#define TEX_FLOOR_2   5
#define TEX_CEIL      6
#define TEX_BOUNDARY  7
#define TEX_COUNT     8

#define SPR_SIZE   16
#define SPR_MASK   15

#define SPR_FOOD      0
#define SPR_BONUS     1
#define SPR_POISON    2
#define SPR_POWERUP   3
#define SPR_COUNT     4

void tex_init(void);

static inline void tex_get(int id, int tx, int ty,
                           unsigned char *r, unsigned char *g, unsigned char *b);
void spr_get(int id, int sx, int sy,
             unsigned char *r, unsigned char *g, unsigned char *b, int *transparent);
void tex_draw_weapon(int pw, int ph, int tick);

extern unsigned char tex_data[TEX_COUNT * TEX_SIZE * TEX_SIZE * 3];

static inline void tex_get(int id, int tx, int ty,
                           unsigned char *r, unsigned char *g, unsigned char *b) {
    int off = (id * TEX_SIZE * TEX_SIZE + (ty & TEX_MASK) * TEX_SIZE + (tx & TEX_MASK)) * 3;
    *r = tex_data[off];
    *g = tex_data[off + 1];
    *b = tex_data[off + 2];
}

#endif
