/*
 * keyboard.c — non-blocking keyboard input with escape sequence parsing.
 *
 * Raw mode via termios: no echo, no line buffering, no signal keys.
 * select() for non-blocking reads. Parses arrow key escape sequences
 * (\033[A/B/C/D) and returns KB_UP/DOWN/LEFT/RIGHT constants.
 */

#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include "screen.h"

static struct termios original_settings;

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

void kb_restore(void) {
    tcsetattr(0, TCSAFLUSH, &original_settings);
}

int kb_read(void) {
    struct timeval tv;
    fd_set fds;
    char c, seq;

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(0, &fds);

    if (select(1, &fds, 0, 0, &tv) <= 0)
        return 0;

    if (read(0, &c, 1) != 1)
        return 0;

    if (c != '\033')
        return c;

    /* ESC received — try to read escape sequence */
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    if (select(1, &fds, 0, 0, &tv) <= 0)
        return 0;

    if (read(0, &seq, 1) != 1 || seq != '[')
        return 0;

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    if (select(1, &fds, 0, 0, &tv) <= 0)
        return 0;

    if (read(0, &seq, 1) != 1)
        return 0;

    if (seq == 'A') return KB_UP;
    if (seq == 'B') return KB_DOWN;
    if (seq == 'C') return KB_RIGHT;
    if (seq == 'D') return KB_LEFT;

    return 0;
}
