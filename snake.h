/* snake.h — declarations for all 5 custom libraries.
   Every .c file includes this one header. */

#ifndef SNAKE_H
#define SNAKE_H

/* ---------- math.c ---------- */
int  abs_val(int x);                           /* absolute value */
int  in_bounds(int x, int y, int w, int h);    /* 1 if inside play area */
int  my_rand(int max);                         /* random 0..max-1 */

/* ---------- string.c ---------- */
int  my_strlen(const char *s);                 /* length of C string */
void int_to_str(int n, char *buf);             /* integer -> text */

/* ---------- memory.c ---------- */
void *my_alloc(int size);                      /* allocate a block */
void  my_free(void *p);                        /* return block to pool */

/* ---------- screen.c ---------- */
void clear_screen(void);
void move_cursor(int row, int col);
void draw_at(int row, int col, char c);
void print_str(int row, int col, const char *s);

/* ---------- keyboard.c ---------- */
void kb_init(void);      /* switch terminal to raw, non-blocking */
void kb_restore(void);   /* put terminal back the way we found it */
int  kb_hit(void);       /* returns key, or 0 if no key pressed */

#endif
