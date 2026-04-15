/*
 * screen.c — terminal rendering using ANSI escape codes.
 *
 * All output goes through write() — we never use printf or putchar.
 * ANSI codes start with ESC (decimal 27, written as \033):
 *   \033[2J     = clear entire screen
 *   \033[H      = move cursor to top-left
 *   \033[r;cH   = move cursor to row r, column c
 *   \033[?25l   = hide the cursor
 *   \033[?25h   = show the cursor
 */

#include <unistd.h>
#include "string.h"

/* Helper: write a null-terminated string to the terminal. */
static void scr_write(const char *s) {
    write(1, s, str_len(s));
}

/* Clear the screen and move cursor to top-left corner. */
void scr_clear(void) {
    scr_write("\033[2J\033[H");
}

/* Hide the blinking cursor (cleaner look during gameplay). */
void scr_hide_cursor(void) {
    scr_write("\033[?25l");
}

/* Show the cursor again (must call this before exiting). */
void scr_show_cursor(void) {
    scr_write("\033[?25h");
}

/*
 * Move the cursor to a specific (row, col) position.
 * Builds the ANSI escape sequence "\033[row;colH" manually
 * using int_to_str from our string library.
 */
void scr_goto(int row, int col) {
    char buf[16] = "\033[";
    int i = 2;

    i += int_to_str(row, buf + i);
    buf[i++] = ';';
    i += int_to_str(col, buf + i);
    buf[i++] = 'H';

    write(1, buf, i);
}

/* Draw a single character at position (row, col). */
void scr_putch(int row, int col, char c) {
    scr_goto(row, col);
    write(1, &c, 1);
}

/* Draw a string at position (row, col). */
void scr_puts(int row, int col, const char *s) {
    scr_goto(row, col);
    write(1, s, str_len(s));
}

/*
 * Draw a border of '#' characters around the game area.
 * The border is (w+2) columns wide and (h+2) rows tall,
 * because it wraps around the w x h playable area.
 */
void scr_draw_box(int w, int h) {
    int i;

    /* Top and bottom rows */
    for (i = 1; i <= w + 2; i++) {
        scr_putch(1, i, '#');
        scr_putch(h + 2, i, '#');
    }

    /* Left and right columns */
    for (i = 2; i <= h + 1; i++) {
        scr_putch(i, 1, '#');
        scr_putch(i, w + 2, '#');
    }
}
