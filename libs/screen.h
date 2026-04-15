#ifndef SCREEN_H
#define SCREEN_H

void scr_clear(void);
void scr_goto(int row, int col);
void scr_putch(int row, int col, char c);
void scr_puts(int row, int col, const char *s);
void scr_hide_cursor(void);
void scr_show_cursor(void);
void scr_draw_box(int w, int h);

#endif
