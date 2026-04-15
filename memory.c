/* memory.c — our own allocator. We never call malloc() or free().
   Idea: reserve one big static "pool" of bytes up front, cut it into
   equal fixed-size blocks, and hand blocks out one at a time.
   A "free list" remembers which blocks are currently unused. */

#include "snake.h"

#define POOL_BLOCKS 512     /* total blocks available        */
#define BLOCK_SIZE   32     /* bytes per block (big enough for a Segment) */

static char pool[POOL_BLOCKS * BLOCK_SIZE];   /* the virtual RAM */
static int  free_list[POOL_BLOCKS];           /* stack of free block indices */
static int  free_top     = -1;                /* top of the free stack */
static int  initialized  = 0;                 /* have we filled the stack yet? */

/* On first use, mark every block as free by pushing its index. */
static void init_pool(void) {
    int i;
    for (i = 0; i < POOL_BLOCKS; i++) {
        free_top++;
        free_list[free_top] = i;
    }
    initialized = 1;
}

/* Return a pointer to a free block, or 0 if pool is empty / too small. */
void *my_alloc(int size) {
    if (!initialized) init_pool();
    if (size > BLOCK_SIZE || free_top < 0) return 0;

    int idx = free_list[free_top];   /* pop top of free stack */
    free_top--;
    return &pool[idx * BLOCK_SIZE];  /* address of that block */
}

/* Compute which block this pointer belongs to and push it back on the stack. */
void my_free(void *p) {
    int idx = (int)(((char *)p - pool) / BLOCK_SIZE);
    free_top++;
    free_list[free_top] = idx;
}
