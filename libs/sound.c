/*
 * Sound library — shells out to macOS's built-in `afplay` via fork+exec.
 *
 * snd_play        : fire-and-forget one-shot (SIGCHLD ignored -> no zombies).
 * snd_music_start : spawns a child that loops afplay forever.
 * snd_music_stop  : kills that looping child.
 */

#include "sound.h"
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

static pid_t music_pid = 0;

/* One-shot sound effect. Parent returns immediately; child execs afplay. */
void snd_play(const char *path) {
    pid_t pid;

    /* Reap automatically so short SFX don't leave zombies. */
    signal(SIGCHLD, SIG_IGN);

    pid = fork();
    if (pid == 0) {
        /* Silence afplay's stdout/stderr so it doesn't corrupt the board. */
        int devnull = open("/dev/null", 1);
        if (devnull >= 0) {
            dup2(devnull, 1);
            dup2(devnull, 2);
            close(devnull);
        }
        char *const argv[] = { "afplay", (char *)path, 0 };
        execvp("afplay", argv);
        _exit(1);
    }
}

/* Background music: child loops afplay forever via /bin/sh.
   The child calls setsid() so it becomes its own process-group leader;
   that lets snd_music_stop kill sh *and* the afplay grandchild together. */
void snd_music_start(const char *path) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        int devnull = open("/dev/null", 1);
        if (devnull >= 0) {
            dup2(devnull, 1);
            dup2(devnull, 2);
            close(devnull);
        }
        char *const argv[] = {
            "sh", "-c", "while :; do afplay \"$0\"; done",
            (char *)path, 0
        };
        execvp("sh", argv);
        _exit(1);
    }
    music_pid = pid;
}

void snd_music_stop(void) {
    if (music_pid > 0) {
        /* Negative pid = signal the whole process group, so afplay dies too. */
        kill(-music_pid, SIGTERM);
        kill(music_pid, SIGTERM);
        music_pid = 0;
    }
}
