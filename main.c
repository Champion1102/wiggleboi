/* main.c — the Snake game itself. Glues together all 5 custom libraries. */

#include <stdio.h>    /* printf for the final game-over line */
#include <unistd.h>   /* usleep for frame delay */
#include "snake.h"

#define WIDTH  40
#define HEIGHT 20

/* A snake segment is a node in a singly linked list of (x,y) positions. */
typedef struct Segment {
    int x, y;
    struct Segment *next;
} Segment;

static Segment *head;         /* front of the snake (where it grows) */
static Segment *tail;         /* back of the snake (where it shrinks) */
static int      dx = 1, dy = 0;  /* current direction vector */
static int      food_x, food_y;
static int      score     = 0;
static int      game_over = 0;

/* Allocate one new segment through our custom memory module. */
static Segment *new_segment(int x, int y) {
    Segment *s = (Segment *)my_alloc(sizeof(Segment));
    s->x = x; s->y = y; s->next = 0;
    return s;
}

static void init_snake(void) {
    head = new_segment(WIDTH / 2, HEIGHT / 2);
    tail = head;                 /* starts as one segment */
}

/* Put food at a random spot inside the walls. */
static void place_food(void) {
    food_x = my_rand(WIDTH  - 3) + 2;
    food_y = my_rand(HEIGHT - 3) + 2;
}

/* Draw the border once at startup. */
static void draw_border(void) {
    int i;
    for (i = 1; i <= WIDTH;  i++) { draw_at(1, i, '#'); draw_at(HEIGHT, i, '#'); }
    for (i = 1; i <= HEIGHT; i++) { draw_at(i, 1, '#'); draw_at(i, WIDTH,  '#'); }
}

/* Check if (x,y) collides with any existing segment (self-bite). */
static int hits_self(int x, int y) {
    Segment *s = head;
    while (s != 0) {
        if (s->x == x && s->y == y) return 1;
        s = s->next;
    }
    return 0;
}

/* Advance the snake by one step in the current direction. */
static void step(void) {
    int nx = head->x + dx;
    int ny = head->y + dy;

    /* Wall collision (uses math.c). */
    if (!in_bounds(nx, ny, WIDTH, HEIGHT)) { game_over = 1; return; }

    /* Self collision. */
    if (hits_self(nx, ny)) { game_over = 1; return; }

    /* Grow new head: put a fresh segment in front, point it at the old head. */
    Segment *n = new_segment(nx, ny);
    n->next = head;
    head = n;

    if (nx == food_x && ny == food_y) {
        score++;
        place_food();            /* ate food -> keep tail, snake grew */
    } else {
        /* No food: erase tail on screen, unlink it, free it. */
        draw_at(tail->y, tail->x, ' ');

        /* Walk to the segment whose ->next is the current tail. */
        Segment *s = head;
        while (s->next != tail) s = s->next;

        my_free(tail);
        tail = s;
        tail->next = 0;
    }
}

/* Redraw the moving pieces + score. Border is static so we skip it. */
static void draw(void) {
    char buf[16];

    draw_at(head->y, head->x, 'O');   /* snake head */
    draw_at(food_y,  food_x,  '*');   /* food */

    int_to_str(score, buf);           /* uses string.c */
    print_str(HEIGHT + 1, 1, "Score: ");
    print_str(HEIGHT + 1, 8, buf);
}

int main(void) {
    kb_init();
    clear_screen();
    draw_border();
    init_snake();
    place_food();

    while (!game_over) {
        /* Read one key this tick. Update direction, but forbid 180° turns. */
        int k = kb_hit();
        if      (k == 'w' && dy == 0) { dx = 0;  dy = -1; }
        else if (k == 's' && dy == 0) { dx = 0;  dy =  1; }
        else if (k == 'a' && dx == 0) { dx = -1; dy =  0; }
        else if (k == 'd' && dx == 0) { dx =  1; dy =  0; }
        else if (k == 'q') break;     /* quit */

        step();
        draw();
        usleep(100000);               /* ~10 frames per second */
    }

    kb_restore();
    move_cursor(HEIGHT + 3, 1);
    printf("Game Over! Final Score: %d\n", score);
    return 0;
}
