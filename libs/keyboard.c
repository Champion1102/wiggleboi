/*
 * keyboard.c — non-blocking keyboard input.
 *
 * Normal terminals wait for Enter before sending input, and they
 * echo every character you type. For a game we need:
 *   1. No waiting for Enter (raw mode via termios)
 *   2. No echo (typing 'w' shouldn't print 'w' on screen)
 *   3. Non-blocking reads (game keeps running even with no input)
 *
 * We use select() to check if a key is available before reading.
 */

#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

static struct termios original_settings;  /* saved to restore on exit */

/*
 * Switch the terminal to raw mode:
 *   ICANON off = don't wait for Enter
 *   ECHO off   = don't show typed characters
 *   ISIG off   = don't handle Ctrl+C as a signal
 */
void kb_raw(void) {
    struct termios raw;

    tcgetattr(0, &original_settings);
    raw = original_settings;

    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(0, TCSAFLUSH, &raw);
}

/* Restore the terminal to its original settings. */
void kb_restore(void) {
    tcsetattr(0, TCSAFLUSH, &original_settings);
}

/*
 * Try to read one key without waiting.
 * Returns the key character, or 0 if no key was pressed.
 *
 * select() with a zero timeout checks if input is available
 * without blocking — if yes, we read it; if no, we return 0
 * and the game loop continues.
 */
int kb_read(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    char c;

    FD_ZERO(&fds);
    FD_SET(0, &fds);

    if (select(1, &fds, 0, 0, &tv) > 0) {
        read(0, &c, 1);
        return c;
    }
    return 0;
}
