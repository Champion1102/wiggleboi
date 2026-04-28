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

#define CH_HALF_UP '\x01'

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
static unsigned char *pixels;
static int pix_w, pix_h;

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
    pix_w = tw;
    pix_h = th * 2;

    total_mem = (unsigned long)ncells * sizeof(struct TCel) * 2
              + 65536
              + (unsigned long)pix_w * pix_h * 3;
    mem = mmap(0, total_mem, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mem == (void *)-1) return;

    front = (struct TCel *)mem;
    back  = (struct TCel *)((char *)mem + (unsigned long)ncells * sizeof(struct TCel));
    wbuf  = (char *)mem + (unsigned long)ncells * sizeof(struct TCel) * 2;
    pixels = (unsigned char *)((char *)mem + (unsigned long)ncells * sizeof(struct TCel) * 2 + 65536);

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

            if (back[idx].ch == CH_HALF_UP) {
                wbuf[p++] = (char)0xE2;
                wbuf[p++] = (char)0x96;
                wbuf[p++] = (char)0x80;
            } else {
                wbuf[p++] = back[idx].ch;
            }
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

void scr_pixel(int x, int y, int r, int g, int b) {
    int idx;
    if (x < 0 || x >= pix_w || y < 0 || y >= pix_h) return;
    idx = (y * pix_w + x) * 3;
    pixels[idx]     = (unsigned char)r;
    pixels[idx + 1] = (unsigned char)g;
    pixels[idx + 2] = (unsigned char)b;
}

void scr_rect(int x, int y, int w, int h, int r, int g, int b) {
    int px, py, x2, y2, idx;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    x2 = x + w; if (x2 > pix_w) x2 = pix_w;
    y2 = y + h; if (y2 > pix_h) y2 = pix_h;
    for (py = y; py < y2; py++)
        for (px = x; px < x2; px++) {
            idx = (py * pix_w + px) * 3;
            pixels[idx]     = (unsigned char)r;
            pixels[idx + 1] = (unsigned char)g;
            pixels[idx + 2] = (unsigned char)b;
        }
}

void scr_vline(int x, int y1, int y2, int r, int g, int b) {
    int y, idx;
    if (x < 0 || x >= pix_w) return;
    if (y1 < 0) y1 = 0;
    if (y2 >= pix_h) y2 = pix_h - 1;
    for (y = y1; y <= y2; y++) {
        idx = (y * pix_w + x) * 3;
        pixels[idx]     = (unsigned char)r;
        pixels[idx + 1] = (unsigned char)g;
        pixels[idx + 2] = (unsigned char)b;
    }
}

void scr_pixel_clear(int r, int g, int b) {
    int i, total;
    total = pix_w * pix_h;
    for (i = 0; i < total; i++) {
        pixels[i * 3]     = (unsigned char)r;
        pixels[i * 3 + 1] = (unsigned char)g;
        pixels[i * 3 + 2] = (unsigned char)b;
    }
}

void scr_pixels_flush(void) {
    int x, y, idx, pt, pb;
    unsigned char tr, tg, tb, br, bg, bb;

    for (y = 0; y < th; y++) {
        for (x = 0; x < tw; x++) {
            pt = ((y * 2) * pix_w + x) * 3;
            pb = ((y * 2 + 1) * pix_w + x) * 3;

            tr = pixels[pt]; tg = pixels[pt + 1]; tb = pixels[pt + 2];
            br = pixels[pb]; bg = pixels[pb + 1]; bb = pixels[pb + 2];

            idx = y * tw + x;

            if (tr == br && tg == bg && tb == bb) {
                back[idx].br = br; back[idx].bg = bg; back[idx].bb = bb;
                back[idx].fr = br; back[idx].fg = bg; back[idx].fb = bb;
                back[idx].ch = ' ';
            } else {
                back[idx].fr = tr; back[idx].fg = tg; back[idx].fb = tb;
                back[idx].br = br; back[idx].bg = bg; back[idx].bb = bb;
                back[idx].ch = CH_HALF_UP;
            }
        }
    }
}

int scr_pixel_w(void) { return pix_w; }
int scr_pixel_h(void) { return pix_h; }

void scr_hide_cursor(void) {
    write(1, "\033[?25l", 6);
}

void scr_show_cursor(void) {
    write(1, "\033[?25h", 6);
}
