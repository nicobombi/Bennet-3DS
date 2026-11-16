/* disegno software su superfici RGB565 */
#include "comune.h"
#include "font.h"

static uint32_t SEME = 0x9E3779B9u;
uint32_t rnd(void) { SEME ^= SEME << 13; SEME ^= SEME >> 17; SEME ^= SEME << 5; return SEME; }
float rndf(void) { return (rnd() >> 8) / 16777216.0f; }

uint16_t mix565(uint16_t a, uint16_t b, int t) {
    int ar = a >> 11, ag = (a >> 5) & 63, ab = a & 31, br = b >> 11, bg = (b >> 5) & 63, bb = b & 31;
    return (uint16_t)(((ar + (((br - ar) * t) >> 8)) << 11) | ((ag + (((bg - ag) * t) >> 8)) << 5) | (ab + (((bb - ab) * t) >> 8)));
}
void sup_riempi(Sup *s, uint16_t c) { for (int i = 0; i < s->w * s->h; i++) s->px[i] = c; }
void sup_rett(Sup *s, int x, int y, int w, int h, uint16_t c) {
    int x0 = clampi(x, 0, s->w), y0 = clampi(y, 0, s->h), x1 = clampi(x + w, 0, s->w), y1 = clampi(y + h, 0, s->h);
    for (int j = y0; j < y1; j++) { uint16_t *p = s->px + j * s->w; for (int i = x0; i < x1; i++) p[i] = c; }
}
void sup_rett_a(Sup *s, int x, int y, int w, int h, uint16_t c, int a) {
    if (a <= 0) return; if (a >= 256) { sup_rett(s, x, y, w, h, c); return; }
    int x0 = clampi(x, 0, s->w), y0 = clampi(y, 0, s->h), x1 = clampi(x + w, 0, s->w), y1 = clampi(y + h, 0, s->h);
    for (int j = y0; j < y1; j++) { uint16_t *p = s->px + j * s->w; for (int i = x0; i < x1; i++) p[i] = mix565(p[i], c, a); }
}
void sup_somma(Sup *s, int x, int y, int w, int h, int r, int g, int b) {
    int x0 = clampi(x, 0, s->w), y0 = clampi(y, 0, s->h), x1 = clampi(x + w, 0, s->w), y1 = clampi(y + h, 0, s->h);
    for (int j = y0; j < y1; j++) { uint16_t *p = s->px + j * s->w; for (int i = x0; i < x1; i++) {
        int c = p[i], R0 = ((c >> 11) << 3) + r, G0 = (((c >> 5) & 63) << 2) + g, B0 = ((c & 31) << 3) + b;
        p[i] = rgb(R0 > 255 ? 255 : R0, G0 > 255 ? 255 : G0, B0 > 255 ? 255 : B0); } }
}
void sup_img_parte(Sup *s, const Img *im, int sx, int sy, int w, int h, int x, int y, int a) {
    if (!im || a <= 0) return;
    for (int j = 0; j < h; j++) {
        int yy = y + j; if (yy < 0 || yy >= s->h) continue;
        const uint16_t *src = im->px + (sy + j) * im->w + sx; uint16_t *dst = s->px + yy * s->w;
        for (int i = 0; i < w; i++) {
            int xx = x + i; if (xx < 0 || xx >= s->w) continue;
            uint16_t c = src[i]; if (c == KEY565) continue;
            dst[xx] = a >= 256 ? c : mix565(dst[xx], c, a);
        }
    }
}
void sup_img(Sup *s, const Img *im, int x, int y, int a) { if (im) sup_img_parte(s, im, 0, 0, im->w, im->h, x, y, a); }
void sup_img_scala(Sup *s, const Img *im, int sx, int sy, int w, int h, int x, int y, int z) {
    if (!im) return;
    for (int j = 0; j < h * z; j++) {
        int yy = y + j; if (yy < 0 || yy >= s->h) continue;
        const uint16_t *src = im->px + (sy + j / z) * im->w + sx; uint16_t *dst = s->px + yy * s->w;
        for (int i = 0; i < w * z; i++) { int xx = x + i; if (xx < 0 || xx >= s->w) continue; uint16_t c = src[i / z]; if (c != KEY565) dst[xx] = c; }
    }
}
void sup_ellisse_ombra(Sup *s, int cx, int cy, int rx, int ry, int a) {
    for (int j = -ry; j <= ry; j++) for (int i = -rx; i <= rx; i++) {
        float d = (float)(i * i) / (rx * rx + .01f) + (float)(j * j) / (ry * ry + .01f); if (d > 1) continue;
        int x = cx + i, y = cy + j; if (x < 0 || y < 0 || x >= s->w || y >= s->h) continue;
        uint16_t *p = s->px + y * s->w + x; *p = mix565(*p, 0, a);
    }
}
void sup_cuore(Sup *s, int x, int y, uint16_t c1, uint16_t c2, int a) {
    static const char *H[] = { ".##.##.", "#######", "#######", ".#####.", "..###..", "...#..." };
    for (int j = 0; j < 6; j++) for (int i = 0; i < 7; i++) if (H[j][i] == '#') sup_rett_a(s, x + i - 3, y + j - 3, 1, 1, (j == 1 && i == 1) || (j == 0 && i == 1) ? c2 : c1, a);
}
void sup_stella(Sup *s, int x, int y, int sz, uint16_t c, int a) {
    sup_rett_a(s, x, y, 1, 1, c, a);
    if (sz > 1) { sup_rett_a(s, x - 1, y, 3, 1, c, a); sup_rett_a(s, x, y - 1, 1, 3, c, a); }
    if (sz > 2) { sup_rett_a(s, x - 2, y, 5, 1, c, a * 6 / 10); sup_rett_a(s, x, y - 2, 1, 5, c, a * 6 / 10); }
}

/* ---------- testo ---------- */
static int utf8(const char **p) {
    const unsigned char *s = (const unsigned char *)*p; int c = *s;
    if (c < 0x80) { *p += 1; return c; }
    if ((c & 0xE0) == 0xC0 && s[1]) { *p += 2; return ((c & 31) << 6) | (s[1] & 63); }
    if ((c & 0xF0) == 0xE0 && s[1] && s[2]) { *p += 3; return ((c & 15) << 12) | ((s[1] & 63) << 6) | (s[2] & 63); }
    *p += 1; return '?';
}
static const Glifo *glifo(int cp) {
    int a = 0, b = FONT_N - 1;
    while (a <= b) { int m = (a + b) / 2; if (FONT[m].cp == cp) return &FONT[m]; if (FONT[m].cp < cp) a = m + 1; else b = m - 1; }
    switch (cp) { case 0x2018: case 0x60: return glifo('\''); case 0x201C: case 0x201D: return glifo('"'); case 0xE1: return glifo('a'); case 0xED: return glifo('i'); case 0xFA: return glifo('u'); }
    return cp == ' ' ? NULL : glifo('?');
}
static int glifo_disegna(Sup *s, int x, int y, const Glifo *g, uint16_t c, int z) {
    if (!g) return 3 * z;
    if (s) for (int j = 0; j < 10; j++) { int r = g->r[j]; if (!r) continue;
        for (int i = 0; i < g->w; i++) if (r & (1 << (g->w - 1 - i))) { if (z == 1) { int xx = x + i, yy = y + j; if (xx >= 0 && yy >= 0 && xx < s->w && yy < s->h) s->px[yy * s->w + xx] = c; } else sup_rett(s, x + i * z, y + j * z, z, z, c); } }
    return (g->w + 1) * z;
}
int testo(Sup *s, int x, int y, const char *t, uint16_t c) {
    int x0 = x; while (*t) { int cp = utf8(&t); if (cp == '\n') break; x += glifo_disegna(s, x, y, cp == ' ' ? NULL : glifo(cp), c, 1); } return x - x0;
}
int testo_largo(const char *t) { return testo(NULL, 0, 0, t, 0); }
int testo_ombra(Sup *s, int x, int y, const char *t, uint16_t c, uint16_t o) { testo(s, x + 1, y + 1, t, o); return testo(s, x, y, t, c); }
int testo_grande(Sup *s, int x, int y, const char *t, uint16_t c, uint16_t o, int z) {
    int x0 = x; const char *p = t;
    if (o != KEY565) { int xx = x; while (*p) { int cp = utf8(&p); xx += glifo_disegna(s, xx + z, y + z, cp == ' ' ? NULL : glifo(cp), o, z); } }
    p = t; while (*p) { int cp = utf8(&p); x += glifo_disegna(s, x, y, cp == ' ' ? NULL : glifo(cp), c, z); }
    return x - x0;
}
/* a capo per parole; maxch limita i caratteri mostrati (macchina da scrivere dei dialoghi) */
static int a_capo(Sup *s, int x, int y, int w, const char *t, uint16_t c, int maxch, bool dis) {
    int cx = x, cy = y, n = 0; const char *p = t;
    while (*p) {
        if (*p == '\n') { p++; cx = x; cy += 11; continue; }
        if (*p == ' ') { const char *q = p; int cp = utf8(&q); (void)cp; if (cx > x) cx += 4; p = q; n++; if (maxch >= 0 && n >= maxch) return cy - y + 11; continue; }
        const char *e = p; int lw = 0;
        while (*e && *e != ' ' && *e != '\n') { const char *q = e; int cp = utf8(&q); lw += glifo_disegna(NULL, 0, 0, glifo(cp), 0, 1); e = q; }
        if (cx + lw > x + w && cx > x) { cx = x; cy += 11; }
        while (p < e) { int cp = utf8(&p); if (maxch >= 0 && n >= maxch) return cy - y + 11; cx += glifo_disegna(dis ? s : NULL, cx, cy, glifo(cp), c, 1); n++; }
    }
    return cy - y + 11;
}
int testo_a_capo(Sup *s, int x, int y, int w, const char *t, uint16_t c, int maxch) { return a_capo(s, x, y, w, t, c, maxch, true); }
int testo_righe(int w, const char *t) { return a_capo(NULL, 0, 0, w, t, 0, -1, false) / 11; }
