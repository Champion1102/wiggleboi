#ifndef SCREEN_H
#define SCREEN_H

#define KB_UP    128
#define KB_DOWN  129
#define KB_RIGHT 130
#define KB_LEFT  131

void scr_init(int game_w, int game_h);
void scr_shutdown(void);

void scr_cell(int gx, int gy, int r, int g, int b);
void scr_border(int tx, int ty, int r, int g, int b);
void scr_text(int row, int col, const char *s, int r, int g, int b);
void scr_fill(int r, int g, int b);
void scr_present(void);

void scr_set_offset(int ox, int oy);
void scr_hide_cursor(void);
void scr_show_cursor(void);

void scr_pixel(int x, int y, int r, int g, int b);
void scr_rect(int x, int y, int w, int h, int r, int g, int b);
void scr_vline(int x, int y1, int y2, int r, int g, int b);
void scr_pixel_clear(int r, int g, int b);
void scr_pixels_flush(void);
int  scr_pixel_w(void);
int  scr_pixel_h(void);

#endif
