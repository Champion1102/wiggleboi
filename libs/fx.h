#ifndef FX_H
#define FX_H

void fx_init(void);
void fx_update(void);

void fx_spawn_eat(int gx, int gy);
void fx_spawn_burst(int gx, int gy, int r, int g, int b, int count);
void fx_shake(int frames);

void fx_trail_push(int gx, int gy);
void fx_trail_clear(void);

void fx_draw_trail(void);
void fx_draw_parts(void);

#endif
