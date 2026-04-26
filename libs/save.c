/*
 * save.c -- high score persistence via ~/.wiggleboi_scores.
 *
 * Binary format: sorted array of ScoreEntry (score + mode).
 * Top 10 scores, written with raw open/read/write/close syscalls.
 */

#include "save.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

static struct ScoreEntry entries[SAVE_MAX];
static int num_entries = 0;
static char filepath[256];
static int path_built = 0;

static void build_path(void) {
    const char *home, *suffix;
    int i, j;

    if (path_built) return;
    home = getenv("HOME");
    suffix = "/.wiggleboi_scores";
    if (!home) home = "/tmp";

    i = 0;
    for (j = 0; home[j] && i < 240; j++)
        filepath[i++] = home[j];
    for (j = 0; suffix[j] && i < 255; j++)
        filepath[i++] = suffix[j];
    filepath[i] = 0;
    path_built = 1;
}

static void save_write(void) {
    int fd;

    build_path();
    fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;
    write(fd, entries, (unsigned long)num_entries * sizeof(struct ScoreEntry));
    close(fd);
}

void save_load(void) {
    int fd, n;

    build_path();
    fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        num_entries = 0;
        return;
    }

    n = (int)read(fd, entries, sizeof(entries));
    close(fd);

    num_entries = n / (int)sizeof(struct ScoreEntry);
    if (num_entries < 0) num_entries = 0;
    if (num_entries > SAVE_MAX) num_entries = SAVE_MAX;
}

int save_is_high(int score) {
    if (score <= 0) return 0;
    if (num_entries == 0) return 1;
    return score > entries[0].score;
}

void save_add(int score, int mode) {
    int i, j;

    if (score <= 0) return;

    for (i = 0; i < num_entries; i++) {
        if (score > entries[i].score) break;
    }

    if (i >= SAVE_MAX) return;

    if (num_entries < SAVE_MAX) num_entries++;
    for (j = num_entries - 1; j > i; j--)
        entries[j] = entries[j - 1];

    entries[i].score = score;
    entries[i].mode = mode;
    save_write();
}

int save_count(void) {
    return num_entries;
}

struct ScoreEntry *save_get(int idx) {
    if (idx < 0 || idx >= num_entries) return 0;
    return &entries[idx];
}
