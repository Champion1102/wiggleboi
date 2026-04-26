#ifndef AI_H
#define AI_H

#define AI_MAX 4

void ai_init(int w, int h);
void ai_reset(void);
int  ai_spawn(int idx);
void ai_update(int tick, int food_x, int food_y, int px, int py, int cur_score);
void ai_draw(int tick);
int  ai_ate_food(void);
int  ai_alive(int idx);
void ai_kill_all(void);

#endif
