/* math.c — tiny replacements for parts of <math.h>.
   We never include <math.h>; everything here is written by hand. */

#include "snake.h"

/* Absolute value: if negative, flip the sign. */
int abs_val(int x) {
    return x < 0 ? -x : x;
}

/* Boundary check: is (x,y) strictly inside the walls of a w x h box?
   The walls themselves sit at 1 and w (or h), so valid play is in between. */
int in_bounds(int x, int y, int w, int h) {
    return x > 1 && x < w && y > 1 && y < h;
}

/* Simple pseudo-random number generator (Linear Congruential Generator).
   Each call mixes the seed with well-known constants, giving a new number. */
static unsigned int seed = 12345;

int my_rand(int max) {
    seed = seed * 1103515245u + 12345u;   /* standard LCG step */
    return (int)((seed / 65536u) % (unsigned)max);
}
