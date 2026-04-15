#ifndef MATH_H
#define MATH_H

void seed_rng(unsigned int s);
int rng_range(int max);
int in_bounds(int x, int y, int w, int h);
int hits_body(int *sx, int *sy, int len, int x, int y);

#endif
