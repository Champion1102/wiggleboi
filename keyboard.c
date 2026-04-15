/* keyboard.c — non-blocking keyboard input on macOS / Linux.
   Normal terminals buffer a whole line until Enter and echo typed keys.
   For a game we want: (1) no line buffering, (2) no echo, (3) read()
   returns immediately even if nothing was typed. */

#include <termios.h>   /* terminal settings struct + tcgetattr/tcsetattr */
#include <unistd.h>    /* read() */
#include <fcntl.h>     /* fcntl() for non-blocking flag */
#include "snake.h"

static struct termios old_settings;  /* saved so we can restore on exit */
static int            old_flags;

void kb_init(void) {
    struct termios n;
    tcgetattr(0, &old_settings);               /* 0 = stdin */
    n = old_settings;
    n.c_lflag &= ~(ICANON | ECHO);             /* off: line buffer + echo */
    tcsetattr(0, TCSANOW, &n);

    old_flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, old_flags | O_NONBLOCK); /* read() won't block */
}

void kb_restore(void) {
    tcsetattr(0, TCSANOW, &old_settings);
    fcntl(0, F_SETFL, old_flags);
}

/* Try to read one byte. If nothing is waiting, read() returns <= 0
   and we return 0 meaning "no key this tick". */
int kb_hit(void) {
    char c;
    int  n = read(0, &c, 1);
    if (n > 0) return (int)c;
    return 0;
}
