/*
 * math.c — game math utilities, built entirely from bitwise primitives.
 *
 * All arithmetic (+, -, *, /, %) is implemented from scratch using only
 * bitwise operators: &, |, ^, ~, <<, >>
 *
 * Provides: bitwise arithmetic, random number generation, boundary checking.
 */

static unsigned int seed;

/* ---- core arithmetic from bitwise operations ---- */

/* addition via ripple carry: XOR gives sum bits, AND+shift gives carry */
int my_add(int a, int b) {
    int carry;
    while (b) {
        carry = a & b;
        a = a ^ b;
        b = carry << 1;
    }
    return a;
}

/* negate via two's complement: -x = ~x + 1 */
int my_negate(int a) {
    return my_add(~a, 1);
}

/* subtraction: a - b = a + (-b) */
int my_sub(int a, int b) {
    return my_add(a, my_negate(b));
}

/* multiplication via shift-and-add */
int my_mul(int a, int b) {
    int result = 0;
    int neg = 0;

    if (a < 0) { a = my_negate(a); neg ^= 1; }
    if (b < 0) { b = my_negate(b); neg ^= 1; }

    while (b) {
        if (b & 1)
            result = my_add(result, a);
        a <<= 1;
        b >>= 1;
    }

    if (neg)
        result = my_negate(result);
    return result;
}

/* division via binary long division (restoring) */
int my_div(int a, int b) {
    unsigned int ua, ub, q, r;
    int neg = 0;
    int i;

    if (b == 0) return 0;

    if (a < 0) { a = my_negate(a); neg ^= 1; }
    if (b < 0) { b = my_negate(b); neg ^= 1; }

    ua = (unsigned int)a;
    ub = (unsigned int)b;
    q = 0;
    r = 0;

    for (i = 31; i >= 0; i = my_add(i, my_negate(1))) {
        r <<= 1;
        r |= (ua >> i) & 1;
        if (r >= ub) {
            r = (unsigned int)my_add((int)r, my_negate((int)ub));
            q |= (1u << i);
        }
    }

    if (neg)
        return my_negate((int)q);
    return (int)q;
}

/* modulo: a % b = a - (a / b) * b */
int my_mod(int a, int b) {
    int q = my_div(a, b);
    return my_sub(a, my_mul(q, b));
}

/* ---- unsigned variants for the LCG (needs wrapping overflow) ---- */

static unsigned int u_add(unsigned int a, unsigned int b) {
    unsigned int carry;
    while (b) {
        carry = a & b;
        a = a ^ b;
        b = carry << 1;
    }
    return a;
}

static unsigned int u_mul(unsigned int a, unsigned int b) {
    unsigned int result = 0;
    while (b) {
        if (b & 1)
            result = u_add(result, a);
        a <<= 1;
        b >>= 1;
    }
    return result;
}

/* ---- random number generation ---- */

void seed_rng(unsigned int s) {
    seed = s;
}

/*
 * LCG random: seed = seed * 1103515245 + 12345
 * All arithmetic uses our bitwise primitives.
 */
int rng_range(int max) {
    seed = u_add(u_mul(seed, 1103515245u), 12345u);
    return my_mod((int)((seed >> 16) & 0x7FFF), max);
}

/* ---- game utilities ---- */

int in_bounds(int x, int y, int w, int h) {
    return x >= 0 && x < w && y >= 0 && y < h;
}

int hits_body(int *sx, int *sy, int len, int x, int y) {
    int i;
    for (i = 0; i < len; i = my_add(i, 1)) {
        if (sx[i] == x && sy[i] == y)
            return 1;
    }
    return 0;
}
