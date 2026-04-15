/* string.c — tiny replacements for parts of <string.h>.
   We never include <string.h>. */

#include "snake.h"

/* Count characters until the C-string null terminator '\0'. */
int my_strlen(const char *s) {
    int n = 0;
    while (s[n] != '\0') n++;
    return n;
}

/* Convert an integer into a character buffer.
   Works by peeling off digits from the right, then reversing. */
void int_to_str(int n, char *buf) {
    char tmp[16];
    int i = 0, j = 0;

    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }

    /* Peel digits: n % 10 gives the last digit; n /= 10 drops it. */
    while (n > 0) {
        tmp[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    /* tmp now has digits in reverse order; copy them back flipped. */
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}
