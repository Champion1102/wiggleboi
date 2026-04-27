#ifndef MATH_H
#define MATH_H

/* bitwise arithmetic — implemented from scratch, no +,-,*,/,% operators */
int my_add(int a, int b);
int my_negate(int a);
int my_sub(int a, int b);
int my_mul(int a, int b);
int my_div(int a, int b);
int my_mod(int a, int b);

void seed_rng(unsigned int s);
int rng_range(int max);
int in_bounds(int x, int y, int w, int h);
int hits_body(int *sx, int *sy, int len, int x, int y);

#endif
