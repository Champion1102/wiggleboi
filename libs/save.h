#ifndef SAVE_H
#define SAVE_H

#define SAVE_MAX 10

struct ScoreEntry {
    int score;
    int mode;
};

void save_load(void);
int  save_is_high(int score);
void save_add(int score, int mode);
int  save_count(void);
struct ScoreEntry *save_get(int idx);

#endif
