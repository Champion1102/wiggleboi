/*
 * Sound library — cross-platform audio via fork+exec.
 *
 * macOS: afplay.  Linux: paplay (PulseAudio) with aplay fallback.
 * snd_play        : fire-and-forget one-shot (SIGCHLD ignored -> no zombies).
 * snd_music_start : spawns a child that loops playback forever.
 * snd_music_stop  : kills that looping child.
 */

#include "sound.h"
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#ifdef __APPLE__
static const char *snd_player = "afplay";
#else
static const char *snd_player = "paplay";
#endif

static pid_t music_pid = 0;

static void silence_output(void) {
    int devnull = open("/dev/null", 1);
    if (devnull >= 0) {
        dup2(devnull, 1);
        dup2(devnull, 2);
        close(devnull);
    }
}

void snd_play(const char *path) {
    pid_t pid;

    signal(SIGCHLD, SIG_IGN);

    pid = fork();
    if (pid == 0) {
        setsid();
        close(0);
        silence_output();
        char *const argv[] = { (char *)snd_player, (char *)path, 0 };
        execvp(snd_player, argv);
        _exit(1);
    }
}

void snd_music_start(const char *path) {
    pid_t pid = fork();
    if (pid == 0) {
        pid_t child;
        int st;
        setsid();
        close(0);
        silence_output();
        signal(SIGCHLD, SIG_DFL);
        for (;;) {
            child = fork();
            if (child == 0) {
#ifdef __APPLE__
                char *const a[] = {"afplay","-v","0.2",(char*)path,0};
                execvp("afplay", a);
#else
                char *const a[] = {"paplay",(char*)path,0};
                execvp("paplay", a);
                char *const b[] = {"aplay","-q",(char*)path,0};
                execvp("aplay", b);
#endif
                _exit(1);
            }
            waitpid(child, &st, 0);
        }
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
