/* string.c — custom string utilities. Replaces <string.h> and sprintf. */

/* Count characters until the null terminator. */
int str_len(const char *s) {
    int n = 0;
    while (s[n])
        n++;
    return n;
}

/*
 * Convert a positive integer to a string.
 * Returns the number of characters written (so the caller
 * knows the length without calling str_len again).
 *
 * How it works:
 *   1. Extract digits from right to left using % 10
 *   2. Reverse the result since digits come out backwards
 */
int int_to_str(int n, char *buf) {
    int i = 0;
    int j;
    char tmp;

    /* Special case: zero */
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }

    /* Extract digits (they come out in reverse order) */
    while (n > 0) {
        buf[i] = '0' + (n % 10);
        n = n / 10;
        i++;
    }
    buf[i] = '\0';

    /* Reverse the string to get the correct order */
    for (j = 0; j < i / 2; j++) {
        tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }

    return i;
}
