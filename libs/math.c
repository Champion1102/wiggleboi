/*
 * math.c — game math utilities. Replaces <math.h> and <stdlib.h>'s rand().
 *
 * Provides: random number generation, boundary checking, collision detection.
 */

static unsigned int seed;

/* Set the random seed. Called once at game start with time(0). */
void seed_rng(unsigned int s) {
    seed = s;
}

/*
 * Generate a random number from 0 to (max - 1).
 * Uses a Linear Congruential Generator (LCG) — the same constants
 * used by many real C libraries. Each call advances the seed.
 */
int rng_range(int max) {
    seed = seed * 1103515245 + 12345;
    return ((seed >> 16) & 0x7FFF) % max;
}

/* Check if position (x, y) is inside the playable area (0-indexed). */
int in_bounds(int x, int y, int w, int h) {
    return x >= 0 && x < w && y >= 0 && y < h;
}

/*
 * Check if position (x, y) overlaps with any snake segment.
 * Used for both self-collision and food placement.
 */
int hits_body(int *sx, int *sy, int len, int x, int y) {
    int i;
    for (i = 0; i < len; i++) {
        if (sx[i] == x && sy[i] == y)
            return 1;
    }
    return 0;
}
