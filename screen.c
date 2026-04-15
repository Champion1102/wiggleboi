/* screen.c — terminal drawing using ANSI escape codes.
   "\033" is the ESC character; terminals interpret ESC[... as commands. */

#include <stdio.h>
#include "snake.h"

/* ESC[2J = clear whole screen. */
void clear_screen(void) {
    printf("\033[2J");
}

/* ESC[row;colH = move the text cursor to (row, col). */
void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

/* Draw a single character at (row, col). fflush forces it out now
   instead of waiting for a newline. */
void draw_at(int row, int col, char c) {
    move_cursor(row, col);
    putchar(c);
    fflush(stdout);
}

/* Print a string at (row, col). We walk it one char at a time using
   my_strlen from our own string.c — so string.c really is used here. */
void print_str(int row, int col, const char *s) {
    int i, len = my_strlen(s);
    move_cursor(row, col);
    for (i = 0; i < len; i++) putchar(s[i]);
    fflush(stdout);
}
