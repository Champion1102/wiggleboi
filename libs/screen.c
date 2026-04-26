/*
 * screen.c — double-buffered truecolor terminal renderer.
 *
 * Each game cell = 2 terminal columns (approximately square).
 * Uses 24-bit RGB via ANSI: \033[48;2;R;G;Bm for background.
 * Double-buffered: diffs back vs front, writes only changed cells.
 * Single write() per frame. Synchronized output prevents tearing.
 */

#include <unistd.h>
#include <sys/mman.h>
#include "string.h"

struct TCel {
    unsigned char br, bg, bb;
    unsigned char fr, fg, fb;
    char ch;
};

static struct TCel *front;
static struct TCel *back;
static char *wbuf;
static int tw, th;
static int ncells;
static int gw, gh;
static int gox, goy;
static int shake_ox, shake_oy;
static unsigned long total_mem;

static int wb_s(int p, const char *s) {
    while (*s) wbuf[p++] = *s++;
    return p;
}

static int wb_i(int p, int n) {
    char tmp[12];
    int len = int_to_str(n, tmp);
    int i;
    for (i = 0; i < len; i++)
        wbuf[p++] = tmp[i];
    return p;
}

void scr_init(int game_w, int game_h) {
    int i;
    void *mem;

    gw = game_w;
    gh = game_h;
    tw = game_w * 2 + 2;
    th = game_h + 4;
    ncells = tw * th;
    gox = 1;
    goy = 1;

    total_mem = (unsigned long)ncells * sizeof(struct TCel) * 2 + 65536;
    mem = mmap(0, total_mem, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mem == (void *)-1) return;

    front = (struct TCel *)mem;
    back  = (struct TCel *)((char *)mem + (unsigned long)ncells * sizeof(struct TCel));
    wbuf  = (char *)mem + (unsigned long)ncells * sizeof(struct TCel) * 2;

    for (i = 0; i < ncells; i++) {
        front[i].br = 1; front[i].bg = 1; front[i].bb = 1;
        front[i].fr = 1; front[i].fg = 1; front[i].fb = 1;
        front[i].ch = '!';
        back[i].br = 0; back[i].bg = 0; back[i].bb = 0;
        back[i].fr = 200; back[i].fg = 200; back[i].fb = 200;
        back[i].ch = ' ';
    }

    write(1, "\033[?1049h", 8);
    write(1, "\033[?25l", 6);
    write(1, "\033[2J\033[H", 7);
}

void scr_shutdown(void) {
    write(1, "\033[0m", 4);
    write(1, "\033[?25h", 6);
    write(1, "\033[?1049l", 8);
    if (front) munmap(front, total_mem);
    front = 0;
}

void scr_set_offset(int ox, int oy) {
    shake_ox = ox;
    shake_oy = oy;
}

void scr_cell(int gx, int gy, int r, int g, int b) {
    int tx, ty, idx;

    tx = gox + gx * 2 + shake_ox;
    ty = goy + gy + shake_oy;
    if (tx < 0 || tx + 1 >= tw || ty < 0 || ty >= th) return;

    idx = ty * tw + tx;
    back[idx].br = r; back[idx].bg = g; back[idx].bb = b;
    back[idx].ch = ' ';
    idx++;
    back[idx].br = r; back[idx].bg = g; back[idx].bb = b;
    back[idx].ch = ' ';
}

void scr_border(int tx, int ty, int r, int g, int b) {
    int idx;
    if (tx < 0 || tx >= tw || ty < 0 || ty >= th) return;
    idx = ty * tw + tx;
    back[idx].br = r; back[idx].bg = g; back[idx].bb = b;
    back[idx].ch = ' ';
}

void scr_text(int row, int col, const char *s, int r, int g, int b) {
    while (*s && col >= 0 && col < tw && row >= 0 && row < th) {
        int idx = row * tw + col;
        back[idx].fr = r; back[idx].fg = g; back[idx].fb = b;
        back[idx].br = 0; back[idx].bg = 0; back[idx].bb = 0;
        back[idx].ch = *s;
        s++;
        col++;
    }
}

void scr_fill(int r, int g, int b) {
    int i;
    for (i = 0; i < ncells; i++) {
        back[i].br = r; back[i].bg = g; back[i].bb = b;
        back[i].fr = 200; back[i].fg = 200; back[i].fb = 200;
        back[i].ch = ' ';
    }
}

void scr_present(void) {
    int p, x, y, idx;
    int pbr, pbg_, pbb, pfr, pfg_, pfb;
    int cr, cc;

    p = 0;
    pbr = -1; pbg_ = -1; pbb = -1;
    pfr = -1; pfg_ = -1; pfb = -1;
    cr = -1; cc = -1;

    p = wb_s(p, "\033[?2026h");

    for (y = 0; y < th; y++) {
        for (x = 0; x < tw; x++) {
            idx = y * tw + x;

            if (back[idx].br == front[idx].br &&
                back[idx].bg == front[idx].bg &&
                back[idx].bb == front[idx].bb &&
                back[idx].fr == front[idx].fr &&
                back[idx].fg == front[idx].fg &&
                back[idx].fb == front[idx].fb &&
                back[idx].ch == front[idx].ch)
                continue;

            if (cr != y || cc != x) {
                p = wb_s(p, "\033[");
                p = wb_i(p, y + 1);
                wbuf[p++] = ';';
                p = wb_i(p, x + 1);
                wbuf[p++] = 'H';
            }

            if ((int)back[idx].br != pbr ||
                (int)back[idx].bg != pbg_ ||
                (int)back[idx].bb != pbb) {
                p = wb_s(p, "\033[48;2;");
                p = wb_i(p, back[idx].br);
                wbuf[p++] = ';';
                p = wb_i(p, back[idx].bg);
                wbuf[p++] = ';';
                p = wb_i(p, back[idx].bb);
                wbuf[p++] = 'm';
                pbr = back[idx].br;
                pbg_ = back[idx].bg;
                pbb = back[idx].bb;
            }

            if (back[idx].ch != ' ') {
                if ((int)back[idx].fr != pfr ||
                    (int)back[idx].fg != pfg_ ||
                    (int)back[idx].fb != pfb) {
                    p = wb_s(p, "\033[38;2;");
                    p = wb_i(p, back[idx].fr);
                    wbuf[p++] = ';';
                    p = wb_i(p, back[idx].fg);
                    wbuf[p++] = ';';
                    p = wb_i(p, back[idx].fb);
                    wbuf[p++] = 'm';
                    pfr = back[idx].fr;
                    pfg_ = back[idx].fg;
                    pfb = back[idx].fb;
                }
            }

            wbuf[p++] = back[idx].ch;
            cr = y;
            cc = x + 1;
            front[idx] = back[idx];

            if (p > 60000) {
                write(1, wbuf, p);
                p = 0;
            }
        }
    }

    p = wb_s(p, "\033[0m\033[?2026l");
    if (p > 0) write(1, wbuf, p);
}

void scr_hide_cursor(void) {
    write(1, "\033[?25l", 6);
}

void scr_show_cursor(void) {
    write(1, "\033[?25h", 6);
}
