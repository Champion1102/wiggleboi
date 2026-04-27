#ifndef RAY_H
#define RAY_H

void ray_init(int screen_w, int screen_h, int grid_w, int grid_h);
void ray_set_camera(int pos_x, int pos_y, int angle);
void ray_set_head(int head_id);
void ray_cast(void);
void ray_draw_walls(void);
void ray_draw_sprites(int tick);
int  ray_depth_at(int col);

#endif
