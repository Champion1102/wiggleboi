/*
 * Snake Game — built entirely from scratch in C.
 *
 * Five custom libraries handle everything (no printf, malloc, etc.):
 *   memory.c   — allocator using mmap() system call
 *   string.c   — string length and integer-to-string conversion
 *   math.c     — random numbers, bounds checking, collision detection
 *   screen.c   — ANSI escape code rendering via write() system call
 *   keyboard.c — non-blocking raw terminal input
 *
 * Controls: W/A/S/D to move, Q to quit.
 * The snake grows when it eats food (*). Game ends on wall or self collision.
 */

#include "libs/memory.h"
#include "libs/screen.h"
#include "libs/keyboard.h"
#include "libs/string.h"
#include "libs/math.h"
#include <unistd.h>
#include <time.h>

#define WIDTH     40     /* game board width  (playable columns) */
#define HEIGHT    20     /* game board height (playable rows)    */
#define MAX_LEN   800    /* maximum snake length (WIDTH * HEIGHT) */

/* ---- Game state (global so helper functions can access it) ---- */

static int *snake_x;           /* x position of each segment */
static int *snake_y;           /* y position of each segment */
static int  snake_len;         /* current number of segments  */

static int dir_x = 1;         /* horizontal direction: -1, 0, or 1 */
static int dir_y = 0;         /* vertical direction:   -1, 0, or 1 */

static int food_x, food_y;    /* current food position */
static int score     = 0;
static int game_over = 0;

/*
 * Place food at a random spot that does not overlap the snake.
 * Uses a do-while loop to keep trying until a valid spot is found.
 */
static void place_food(void) {
    do {
        food_x = rng_range(WIDTH) + 1;
        food_y = rng_range(HEIGHT) + 1;
    } while (hits_body(snake_x, snake_y, snake_len, food_x, food_y));

    scr_putch(food_y + 1, food_x + 1, '*');
}

/*
 * Allocate memory and set the snake to 3 segments in the center.
 * Array layout: index 0 = tail, index [snake_len - 1] = head.
 */
static void init_game(void) {
    int i;

    snake_x = mem_alloc(MAX_LEN * sizeof(int));
    snake_y = mem_alloc(MAX_LEN * sizeof(int));
    snake_len = 3;

    for (i = 0; i < snake_len; i++) {
        snake_x[i] = WIDTH / 2 - 2 + i;
        snake_y[i] = HEIGHT / 2;
    }

    seed_rng((unsigned int)time(0));
}

/*
 * Set up the terminal and draw the first frame:
 * border, initial snake, food, and score.
 *
 * Note: game coordinates (1..WIDTH, 1..HEIGHT) map to terminal
 * coordinates with a +1 offset because the border occupies row/col 1.
 */
static void draw_initial(void) {
    int i;

    kb_raw();
    scr_hide_cursor();
    scr_clear();
    scr_draw_box(WIDTH, HEIGHT);

    /* Draw each segment: body = 'o', head (last segment) = '@' */
    for (i = 0; i < snake_len; i++) {
        char ch = (i == snake_len - 1) ? '@' : 'o';
        scr_putch(snake_y[i] + 1, snake_x[i] + 1, ch);
    }

    place_food();
    scr_puts(HEIGHT + 3, 2, "Score: 0");
}

/*
 * Read one key from the keyboard. Update direction if valid.
 * Prevents 180-degree turns (e.g. can't go left if already going right).
 * Returns 0 if the player pressed 'q' to quit, 1 otherwise.
 */
static int handle_input(void) {
    int key = kb_read();

    if (key == 'q')
        return 0;

    if (key == 'w' && dir_y != 1)  { dir_x =  0; dir_y = -1; }
    if (key == 's' && dir_y != -1) { dir_x =  0; dir_y =  1; }
    if (key == 'a' && dir_x != 1)  { dir_x = -1; dir_y =  0; }
    if (key == 'd' && dir_x != -1) { dir_x =  1; dir_y =  0; }

    return 1;
}

/*
 * Advance the snake by one step. Handles:
 *   - wall collision (game over)
 *   - eating food (grow + new food)
 *   - self collision (game over)
 *   - rendering the moved segments
 */
static void move_snake(void) {
    int nx = snake_x[snake_len - 1] + dir_x;
    int ny = snake_y[snake_len - 1] + dir_y;
    int ate, i;
    char score_buf[12];

    /* Wall collision check */
    if (!in_bounds(nx, ny, WIDTH, HEIGHT)) {
        game_over = 1;
        return;
    }

    /* Did the head land on food? */
    ate = (nx == food_x && ny == food_y);

    if (!ate) {
        /* No food: erase tail from screen, then shift the body forward.
           This removes the tail and makes room for the new head position. */
        scr_putch(snake_y[0] + 1, snake_x[0] + 1, ' ');
        for (i = 0; i < snake_len - 1; i++) {
            snake_x[i] = snake_x[i + 1];
            snake_y[i] = snake_y[i + 1];
        }
    } else {
        /* Ate food: skip the shift so the tail stays — the snake grows. */
        snake_len++;
    }

    /* Self collision check (done after tail removal so we don't
       falsely collide with a tail that was about to disappear). */
    if (hits_body(snake_x, snake_y, snake_len - 1, nx, ny)) {
        game_over = 1;
        return;
    }

    /* Place the new head at the front of the array */
    snake_x[snake_len - 1] = nx;
    snake_y[snake_len - 1] = ny;

    /* Render: turn the old head into a body 'o', draw new head '@' */
    if (snake_len > 1)
        scr_putch(snake_y[snake_len - 2] + 1, snake_x[snake_len - 2] + 1, 'o');
    scr_putch(ny + 1, nx + 1, '@');

    /* If we ate food, update score and spawn new food */
    if (ate) {
        score++;
        place_food();
        int_to_str(score, score_buf);
        scr_puts(HEIGHT + 3, 9, score_buf);
    }
}

/* Restore the terminal to its original state and free memory. */
static void cleanup(void) {
    scr_puts(HEIGHT / 2 + 1, WIDTH / 2 - 3, "GAME OVER");
    usleep(2000000);

    scr_show_cursor();
    scr_clear();
    kb_restore();

    mem_free(snake_x, MAX_LEN * sizeof(int));
    mem_free(snake_y, MAX_LEN * sizeof(int));
}

/* ---- Main game loop ---- */

int main(void) {
    init_game();
    draw_initial();

    while (!game_over) {
        if (!handle_input())
            break;

        move_snake();
        usleep(100000);   /* ~10 frames per second */
    }

    cleanup();
    return 0;
}
