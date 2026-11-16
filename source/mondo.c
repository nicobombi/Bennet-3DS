/* schermo sopra: terreno, oggetti e personaggi ordinati per profondità, luci della notte, vignettatura, dissolvenze */
#include "stato.h"
#include "dati.h"
#include "atmo.h"

static int CX, CY;                         /* angolo in alto a sinistra della vista, in pixel del mondo */
static uint8_t *ALFA; static int ALFA_N;   /* trasparenza delle chiome (per oggetto) */

void mondo_entra(void) {
    Mappa *m = M_ATT(); free(ALFA); ALFA_N = m->nist; ALFA = malloc(ALFA_N ? ALFA_N : 1); memset(ALFA, 255, ALFA_N);
}

/* ---------- sprite degli oggetti: righe RLE di indici nella palette ---------- */
/* sw = spostamento in pixel della cima (vento), che cala verso la base come nelle piante del PC */
static void spr_blit_v(Sup *s, int id, int x, int y, int a, int taglio, float sw, int base, float esp) {
    const SprInfo *si = &R.spr[id]; const uint8_t *p = R.sdat + si->off;
    bool vento = sw > .3f || sw < -.3f; if (base < 1) base = 1;
    for (int j = 0; j < si->h; j++) {
        int n = *p++, yy = y + j, xx = x;
        if (vento && j < base) xx += (int)lroundf(sw * powf((float)(base - j) / base, esp));
        bool vis = yy >= 0 && yy < s->h; int aa = j >= taglio ? 256 : a;
        for (int r = 0; r < n; r++) {
            xx += p[0]; int len = p[1]; const uint8_t *px = p + 2; p += 2 + len;
            if (!vis) { xx += len; continue; }
            uint16_t *d = s->px + yy * s->w;
            int i0 = xx < 0 ? -xx : 0, i1 = xx + len > s->w ? s->w - xx : len;
            if (aa >= 256) for (int i = i0; i < i1; i++) d[xx + i] = R.spal[px[i]];
            else for (int i = i0; i < i1; i++) d[xx + i] = mix565(d[xx + i], R.spal[px[i]], aa);
            xx += len;
        }
    }
}
static void spr_blit(Sup *s, int id, int x, int y, int a, int taglio) { spr_blit_v(s, id, x, y, a, taglio, 0, 1, 1); }
bool sprite_opaco(int id, int x, int y) {
    const SprInfo *si = &R.spr[id]; if (x < 0 || y < 0 || x >= si->w || y >= si->h) return false;
    const uint8_t *p = R.sdat + si->off;
    for (int j = 0; j < y; j++) { int n = *p++; for (int r = 0; r < n; r++) p += 2 + p[1]; }
    int n = *p++, xx = 0;
    for (int r = 0; r < n; r++) { xx += p[0]; if (x >= xx && x < xx + p[1]) return true; xx += p[1]; p += 2 + p[1]; }
    return false;
}

/* ---------- terreno ---------- */
static void terreno(Sup *s) {
    Mappa *m = M_ATT();
    const uint8_t *g0 = NULL, *gk = NULL; int gmask = R.gN - 1;
    if (R.gfr && !m->interno && UI.effetti) { size_t fs = (size_t)R.gN * R.gN; g0 = R.gfr; gk = R.gfr + (size_t)((int)(A.windT * R.gF) % R.gF) * fs; }
    int tx0 = CX >> 4, ty0 = CY >> 4; if (CX < 0) tx0 = -((-CX + 15) >> 4); if (CY < 0) ty0 = -((-CY + 15) >> 4);
    for (int ty = ty0; ty * T < CY + SH; ty++) {
        if (ty < 0 || ty >= m->H) continue;
        for (int tx = tx0; tx * T < CX + SW; tx++) {
            if (tx < 0 || tx >= m->W) continue;
            const uint8_t *src = R.tiles + (size_t)m->tmap[ty * m->W + tx] * 256;
            int sx = tx * T - CX, sy = ty * T - CY;
            int i0 = sx < 0 ? -sx : 0, i1 = sx + 16 > SW ? SW - sx : 16, j0 = sy < 0 ? -sy : 0, j1 = sy + 16 > SH ? SH - sy : 16;
            int tid = m->tmap[ty * m->W + tx];
            if (gk && (R.tflag[tid] & 1)) {                       /* erba al vento: dove il terreno mostra l'erba «pura», il fotogramma giusto */
                for (int j = j0; j < j1; j++) {
                    uint16_t *d = s->px + (sy + j) * SW + sx; const uint8_t *r = src + j * 16; int wy = ((ty * T + j) & gmask) * R.gN;
                    for (int i = i0; i < i1; i++) { int ti = r[i], q = wy + ((tx * T + i) & gmask); d[i] = R.glut[g0[q]] == ti ? R.gpal[gk[q]] : R.tpal[ti]; }
                }
            } else for (int j = j0; j < j1; j++) { uint16_t *d = s->px + (sy + j) * SW + sx; const uint8_t *r = src + j * 16; for (int i = i0; i < i1; i++) d[i] = R.tpal[r[i]]; }
        }
    }
    /* acqua dei laghetti: il fotogramma corrente sopra il primo, «cotto» nel terreno */
    for (int k = 0; k < R.nlaghi && UI.effetti; k++) {
        struct Lago *L = &R.laghi[k]; if (L->mappa != S.mappa || L->F < 2) continue;
        int f = (int)(A.fxT * L->fps) % L->F; if (f == 0) continue;
        int x0 = clampi(L->x - CX, 0, SW), x1 = clampi(L->x + L->w - CX, 0, SW), y0 = clampi(L->y - CY, 0, SH), y1 = clampi(L->y + L->h - CY, 0, SH);
        size_t fs = (size_t)L->w * L->h; const uint8_t *f0 = L->fr, *fk = L->fr + f * fs;
        for (int y = y0; y < y1; y++) {
            size_t o = (size_t)(y + CY - L->y) * L->w + (CX - L->x); uint16_t *d = s->px + y * SW;
            for (int x = x0; x < x1; x++) if (f0[o + x]) d[x] = L->pal[fk[o + x]];
        }
    }
}

/* ---------- personaggi ---------- */
static const int CYCLE[4] = { 1, 0, 3, 0 };
static void foglio(Sup *s, const char *nome, int dir, int f, int x, int y, int a) {
    const Img *im = img(nome); if (im) sup_img_parte(s, im, f * 16, dir * 32, 16, 32, x, y, a);
}
static void cuore(Sup *s, int x, int y, uint16_t c1, uint16_t c2, int a) { sup_cuore(s, x + 2, y + 3, c1, c2, a); }

static void disegna_player(Sup *s) {
    int px = (int)lroundf(S.px) - CX, py = (int)lroundf(S.py) - CY;
    sup_ellisse_ombra(s, px, py - 1, 6, 2, 90);
    if (S.festa < FESTA_DUR) {                               /* festa: saltella e alza la Shota Dorata */
        float e = S.festa, salto = fabsf(sinf((e + .25f) * 3.14159f * 1.9f)) * (e < 3.4f ? 1 : 1 - (e - 3.4f) / 1.2f);
        int dy = (int)lroundf(salto * 5);
        foglio(s, "player", D_GIU, 0, px - 8, py - 31 - dy, 256);
        if (e < 3.6f) {
            int x = px - 8, y = (int)lroundf(py - 31 - dy - 17 - fminf(3, e * 6));
            for (int r = 18; r > 2; r -= 3) sup_somma(s, x + 8 - r / 2, y + 8 - r / 2, r, r, 18, 14, 4);
            for (int k = 0; k < 6; k++) {
                float an = e * 1.4f + k * 1.0472f, r2 = 14 + sinf(e * 4 + k) * 2;
                for (float r = 9; r < r2; r += 1) sup_rett_a(s, (int)(x + 8 + cosf(an) * r), (int)(y + 8 + sinf(an) * r), 1, 1, rgb(255, 229, 138), 115);
            }
            int it = item_cerca("shotaDorata"); const ItemDef *d = &ITEMS[it];
            const Img *im = d->frames ? d->fr[(int)(e * 9) % d->frames] : d->icona; sup_img(s, im, x, y, 256);
        }
        return;
    }
    int f = S.moving ? 1 + (int)(S.dist / 8) % 4 : 0;
    foglio(s, "player", S.dir, f, px - 8, py - 31, 256);
}

static void disegna_siro(Sup *s) {
    float t = S.t; int X = SI.tx * T - CX, Y = (SI.ty + 1) * T - 32 - CY;
    if (S.q_stato == Q_FATTA) {
        float e = S.festa;
        if (e < FESTA_DUR) {
            float salto = fabsf(sinf(e * 3.14159f * 2.1f)); int dy = (int)lroundf(salto * 7);
            sup_rett_a(s, X + 4 + dy / 3, Y + 30, 8 - dy / 2, 2, 0, 90);
            sup_img(s, img(salto > .25f ? "siro_evviva" : "siro_down"), X, Y - dy, 256);
            for (int i = 0; i < 9; i++) {
                float u = e - i * .42f; if (u < 0 || u > 1.5f) continue; float k = u / 1.5f;
                cuore(s, X + 6 + (int)lroundf(sinf(u * 5 + i) * 7) + (i % 3 - 1) * 4, Y - 4 - (int)lroundf(k * 26), i % 2 ? rgb(255, 93, 143) : rgb(255, 59, 92), rgb(255, 209, 223), (int)((1 - k * k) * 256));
            }
            if (e < 1.1f) for (int i = 0; i < 14; i++) {
                float an = i / 14.f * 6.2832f, r = 4 + e * 30, k = e / 1.1f;
                static const uint16_t C[4] = { 0xFFB8, 0xFEAB, 0xFFFF, 0xFD9A };
                sup_stella(s, (int)lroundf(X + 8 + cosf(an) * r), (int)lroundf(Y + 10 + sinf(an) * r * .7f), 3 - (int)(k * 3), C[i % 4], (int)((1 - k) * 256));
            }
            return;
        }
        float u = fmodf(t + SI.tx, 7); int hop = u < .5f ? (int)lroundf(sinf(u / .5f * 3.14159f) * 3) : 0;
        sup_img(s, img(u > .1f && u < .45f ? "siro_evviva" : "siro_down"), X, Y - hop, 256);
        if (u > .3f && u < 1.6f) { float k = (u - .3f) / 1.3f; cuore(s, X + 10, Y - 2 - (int)lroundf(k * 10), rgb(255, 93, 143), rgb(255, 209, 223), (int)((1 - k) * 256)); }
        return;
    }
    /* ciclo di 16 s: inginocchiato a sbirciare tra le grate, poi si alza agitato e si guarda intorno */
    static const struct { const char *f; float d; } GIRO[] = { {"left", .55f}, {"down", .45f}, {"right", .6f}, {"up", .35f}, {"desp", 1.1f}, {"right", .4f}, {"down", .5f}, {"left", .45f} };
    float L = 16, u = fmodf(t + (SI.tx * 1.7f + SI.ty * .9f), L);
    const char *f = "crouch"; bool piedi = false; float v = 0;
    if (u < 8.5f) { int k = (int)(u / .7f); f = (k % 5 == 2 || k % 5 == 3) ? "peer" : "kneel"; }
    else if (u >= 8.9f) { v = u - 8.9f; for (int i = 0; i < 8; i++) { if (v < GIRO[i].d) { f = GIRO[i].f; piedi = true; break; } v -= GIRO[i].d; } }
    int trema = piedi && ((int)(t * 9) % 4 == 0) ? 1 : 0;
    char nm[24]; snprintf(nm, sizeof nm, "siro_%s", f);
    sup_img(s, img(nm), X + (!strcmp(f, "desp") ? (int)(t * 14) % 2 : 0), Y - trema, 256);
    if (piedi) {
        float k = fmodf(t * 1.6f, 1); int gx = X + (!strcmp(f, "right") ? 4 : 12), gy = Y + 7 + (int)lroundf(k * 5) - trema, a = (int)((1 - k * .6f) * 256);
        sup_rett_a(s, gx, gy, 1, 2, rgb(159, 216, 255), a); sup_rett_a(s, gx, gy, 1, 1, 0xFFFF, a); sup_rett_a(s, gx, gy + 2, 1, 1, rgb(74, 138, 192), a);
    }
    if (!strcmp(f, "desp") || (piedi && !strcmp(f, "left") && v < .25f)) {
        int bx = X + 7, by = Y - 6 + (int)(t * 6) % 2;
        sup_rett(s, bx - 1, by - 1, 4, 9, rgb(42, 20, 8)); sup_rett(s, bx, by, 2, 5, rgb(255, 241, 191)); sup_rett(s, bx, by + 6, 2, 1, rgb(255, 241, 191));
    }
}

static void disegna_zambu(Sup *s) {
    if (ZB.stato == ZB_VIA) return;
    float t = S.t;
    if (ZB.stato != ZB_FERMO) {
        int f = (ZB.dir == D_GIU || ZB.dir == D_SU) ? CYCLE[(int)(ZB.passo * 7) % 4] : 1 + (int)(ZB.passo * 7) % 4;
        int a = (int)(ZB.alpha * 256); sup_ellisse_ombra(s, (int)ZB.x - CX, (int)ZB.y - CY - 1, 6, 2, (int)(90 * ZB.alpha));
        foglio(s, "zambu", ZB.dir, f, (int)lroundf(ZB.x - 8) - CX, (int)lroundf(ZB.y - 32) - CY, a); return;
    }
    int X = ZB.tx * T - CX, Y = (ZB.ty + 1) * T - 32 - CY;
    float off = fmodf(ZB.tx * 1.37f + ZB.ty * .71f, 8), u = fmodf(t + off, 8); bool su = u > 3.6f && u < 5.2f;
    sup_ellisse_ombra(s, X + 8, Y + 31, 6, 2, 90);
    sup_img(s, img(su ? "zambu_su" : "zambu_giu"), X, Y, 256);
    const int *b = su ? ZB_BRACE_SU : ZB_BRACE_GIU; uint16_t col = rgb(184, 64, 28);
    if (su) { float k = fminf(1, (u - 3.6f) / 1.2f); col = k > .7f ? rgb(255, 224, 138) : k > .35f ? rgb(255, 176, 74) : rgb(255, 122, 42); }
    else if (sinf(t * 3.1f + ZB.tx) > .6f) col = rgb(217, 88, 42);
    sup_rett(s, X + b[0], Y + b[1], 1, 1, col);
    if (su && u > 4.2f) sup_rett_a(s, X + b[0] - 1, Y + b[1] - 1, 3, 3, rgb(255, 154, 60), 90);
    for (int k = 0; k < 8; k++) {                           /* un filo di fumo che sale dalla brace */
        float age = fmodf(t * .9f + k * .41f, 3.3f), a = age / 3.3f;
        int fx = X + b[0] + (int)lroundf(sinf(age * 2.4f + k) * (.5f + a * 3) + a * a * 6), fy = Y + b[1] - 1 - (int)lroundf(age * 8 + a * a * 5);
        int sz = 1 + (int)(a * 1.6f); sup_rett_a(s, fx - sz / 2, fy - sz / 2, sz, sz, rgb(217, 219, 222), (int)(fminf(1, age / .15f) * (1 - a) * .6f * 256));
    }
}

static void disegna_andrea(Sup *s) {
    bool fermo = PS.pausa > 0 || PS.parla || !PS.dist;
    int f = fermo ? 0 : (PS.dir == D_GIU || PS.dir == D_SU ? CYCLE[(int)(PS.dist / 7) % 4] : 1 + (int)(PS.dist / 7) % 4);
    int x = (int)lroundf(PS.x) - CX, y = (int)lroundf(PS.y) - CY;
    sup_ellisse_ombra(s, x, y - 1, 6, 2, 90);
    foglio(s, "andrea", PS.dir, f, x - 8, y - 31, 256);
}
static void disegna_cane(Sup *s) {
    Cane *C = &PS.cane; static const int RIGA[4] = { 0, 2, 3, 1 };   /* giù, sinistra, destra, su → righe del foglio */
    int cel = C->fermo ? 0 : 1 + (int)(C->dist / 3.2f) % 4, x = (int)lroundf(C->x) - CX, y = (int)lroundf(C->y) - CY;
    sup_rett_a(s, x - 4, y - 1, 9, 2, 0, 56); sup_rett_a(s, x - 3, y + 1, 7, 1, 0, 56);
    const Img *im = img("cane"); if (im) sup_img_parte(s, im, cel * 18, RIGA[C->dir] * 16, 18, 16, x - 9, y - 14, 256);
    if (PS.cuori > 0) { float k = 1 - PS.cuori / 1.8f; for (int i = 0; i < 3; i++) { float u = fmodf(k + i * .3f, 1);
        cuore(s, (int)lroundf(C->x - 4 + i * 4 + sinf(u * 6 + i) * 1.5f) - CX, (int)lroundf(C->y - 15 - u * 10) - CY, rgb(255, 107, 138), rgb(255, 208, 220), (int)(fmaxf(0, 1 - u) * 256)); } }
}
static void guinzaglio(Sup *s) {
    static const int MANO[4][2] = { {4, -10}, {-2, -10}, {2, -10}, {-4, -10} }, COLLO[4][2] = { {-1, -8}, {-3, -6}, {3, -6}, {0, -9} };
    float x0 = PS.x + MANO[PS.dir][0], y0 = PS.y + MANO[PS.dir][1], x1 = PS.cane.x + COLLO[PS.cane.dir][0], y1 = PS.cane.y + COLLO[PS.cane.dir][1];
    float L = hypotf(x1 - x0, y1 - y0), sag = fmaxf(0, 7 - L * .35f); int n = (int)fmaxf(2, ceilf(L));
    for (int i = 0; i <= n; i++) { float t = (float)i / n; sup_rett(s, (int)lroundf(x0 + (x1 - x0) * t) - CX, (int)lroundf(y0 + (y1 - y0) * t + sag * 4 * t * (1 - t)) - CY, 1, 1, rgb(59, 42, 34)); }
}

/* oggetti a terra da raccogliere: l'icona e un luccichio ogni tanto */
static void raccoglibili(Sup *s) {
    Mappa *m = M_ATT();
    for (int i = 0; i < m->nracc && i < 8; i++) {
        if (S.raccolti[S.mappa] & (1 << i)) continue;
        Racc *o = &m->racc[i]; int x = o->x * T - CX, y = o->y * T - CY;
        if (x < -16 || y < -16 || x > SW || y > SH) continue;
        float u = fmodf(S.t + o->x * .37f, 2.4f); if (u > .5f) continue;
        float k = sinf(u / .5f * 3.14159f); int a = (int)(k * 256);
        sup_rett_a(s, x + 12, y + 3, 1, 1, 0xFFFF, a);
        if (k > .4f) { sup_rett_a(s, x + 11, y + 3, 3, 1, rgb(255, 244, 192), a); sup_rett_a(s, x + 12, y + 2, 1, 3, rgb(255, 244, 192), a); }
    }
    for (int b = 0; b < 4; b++) {
        float e = BRILLI[b].t; if (e <= 0 || e > .7f) continue;
        for (int k = 0; k < 8; k++) { float an = k / 8.f * 6.2832f, r = 3 + e * 22;
            sup_rett_a(s, (int)lroundf(BRILLI[b].x + cosf(an) * r) - CX, (int)lroundf(BRILLI[b].y + sinf(an) * r - e * 6) - CY, 1, 1, rgb(255, 244, 192), (int)((1 - e / .7f) * 256)); }
    }
}

/* ---------- oggetti e personaggi in ordine di profondità ---------- */
enum { E_PLAYER, E_SIRO, E_ZAMBU, E_ANDREA, E_CANE };
typedef struct { int base, k; } Ent;
static void ent_disegna(Sup *s, int k) {
    switch (k) {
    case E_PLAYER: disegna_player(s); break;
    case E_SIRO: disegna_siro(s); break;
    case E_ZAMBU: disegna_zambu(s); break;
    case E_ANDREA: disegna_andrea(s); break;
    case E_CANE: disegna_cane(s); break;
    }
}
static void scena(Sup *s, bool titolo) {
    Mappa *m = M_ATT(); Ent E[6]; int ne = 0;
    if (!titolo) E[ne++] = (Ent){ (int)S.py, E_PLAYER };
    if (SI.c) E[ne++] = (Ent){ (SI.ty + 1) * T, E_SIRO };
    if (ZB.c && ZB.stato != ZB_VIA) E[ne++] = (Ent){ ZB.stato == ZB_FERMO ? (ZB.ty + 1) * T : (int)ZB.y, E_ZAMBU };
    if (PS.ok && S.mappa == 0) { E[ne++] = (Ent){ (int)PS.y, E_ANDREA }; E[ne++] = (Ent){ (int)PS.cane.y, E_CANE }; }
    for (int i = 1; i < ne; i++) for (int j = i; j > 0 && E[j].base < E[j - 1].base; j--) { Ent t = E[j]; E[j] = E[j - 1]; E[j - 1] = t; }
    int r0 = clampi((CY - m->maxgiu) / T - 1, 0, m->H + 2), r1 = clampi((CY + SH + m->maxsu) / T + 2, 0, m->H + 2);
    int k0 = m->riga[r0], k1 = m->riga[r1], e = 0;
    while (e < ne && E[e].base < r0 * T) e++;                 /* personaggi sopra la parte visibile: non si vedono */
    for (int k = k0; k < k1; k++) {
        Ist *o = &m->ist[k];
        while (e < ne && E[e].base < o->base) ent_disegna(s, E[e++].k);
        const SprInfo *si = &R.spr[o->s];
        if (o->x >= CX + SW || o->x + si->w <= CX || o->y >= CY + SH || o->y + si->h <= CY) continue;
        int a = 256;
        if (o->chioma >= 2) { if (S.raccolti[S.mappa] & (1 << ((o->chioma - 2) & 7))) continue; }
        else if (o->chioma && !titolo) {                            /* dietro una chioma: diventa semitrasparente */
            bool tocca = S.py < o->base && (sprite_opaco(o->s, (int)S.px - o->x, (int)S.py - 12 - o->y) || sprite_opaco(o->s, (int)S.px - o->x, (int)S.py - 26 - o->y));
            int tgt = tocca ? 128 : 255, v = ALFA[k]; v += (tgt - v) / 4 + (tgt > v ? 1 : tgt < v ? -1 : 0); ALFA[k] = clampi(v, 128, 255); a = ALFA[k] >= 255 ? 256 : ALFA[k];
        }
        float sw = 0; int bs = o->base - o->y; float esp = 1.4f;
        if (UI.effetti && (o->chioma == 1 || o->chioma >= 16)) {
            float tx = o->x / 16.f, ty = o->base / 16.f, ph = A.fxT * .45f - tx * .07f - ty * .025f;   /* l'onda avanza da ovest verso est */
            float w1 = .5f + .5f * sinf(ph * 6.2832f), w2 = .5f + .5f * sinf(ph * 9.1f + tx * .7f);
            float v = clampf((.2f + .6f * w1 + .2f * w2) * (.55f + .55f * A.gust), 0, 1);
            if (o->chioma >= 16) { sw = roundf(v * 6) / 6 * (o->chioma - 16); bs -= 3; }
            else { sw = (v * 2.2f - .6f) * 1.6f; esp = 1.8f; }
        }
        spr_blit_v(s, o->s, o->x - CX, o->y - CY, a, a < 256 ? o->base - o->y - 10 : 9999, sw * A.dir, bs, esp);
    }
    /* fumo dei camini e dei falò: sopra a tutto, come nel PC */
    for (int i = 0; i < R.nfumi; i++) if (R.fumi[i].mappa == S.mappa) {
        float ox = R.fumi[i].x * T + 8, oy = R.fumi[i].y * T + 8;
        if (ox > CX - 60 && ox < CX + SW + 60 && oy > CY - 20 && oy < CY + SH + 120) fumo_disegna(s, CX, CY, ox, oy, R.fumi[i].sd, R.fumi[i].denso, A.fxT);
    }
    while (e < ne) ent_disegna(s, E[e++].k);
    if (PS.ok && S.mappa == 0) guinzaglio(s);
}

/* ---------- luce: mappa a metà risoluzione, poi moltiplicata sulla scena ---------- */
#define LW (SW / 2)
#define LH (SH / 2)
static int LR[LW * LH], LG[LW * LH], LB[LW * LH];
static uint8_t VIGN[LW * LH]; static bool VIGN_OK;
static void luce(float x, float y, float r, int cr, int cg, int cb, float a) {
    float cx = (x - CX) / 2, cy = (y - CY) / 2, rr = r / 2;
    int x0 = (int)(cx - rr), x1 = (int)(cx + rr) + 1, y0 = (int)(cy - rr), y1 = (int)(cy + rr) + 1;
    if (x1 < 0 || y1 < 0 || x0 >= LW || y0 >= LH || a <= 0) return;
    x0 = clampi(x0, 0, LW - 1); x1 = clampi(x1, 0, LW - 1); y0 = clampi(y0, 0, LH - 1); y1 = clampi(y1, 0, LH - 1);
    float ir = 1 / rr, kr = cr * 16 * a, kg = cg * 16 * a, kb = cb * 16 * a;
    for (int j = y0; j <= y1; j++) for (int i = x0; i <= x1; i++) {
        float dx = i + .5f - cx, dy = j + .5f - cy, d = sqrtf(dx * dx + dy * dy) * ir; if (d >= 1) continue;
        float f = d < .45f ? 1 - .45f * (d / .45f) : .55f * (1 - (d - .45f) / .55f);   /* come il gradiente del gioco: 1 → 0,55 → 0 */
        int q = j * LW + i; LR[q] += (int)(kr * f); LG[q] += (int)(kg * f); LB[q] += (int)(kb * f);
    }
}
/* ombre delle nuvole sulla mappa di luce (come nel PC: colore del cielo, 17% di giorno) */
static void nuvole(void) {
    float al = .17f * A.giorno; if (al < .01f || !UI.effetti) return;
    int k = (int)(al * 256), cr = 26 * 16, cg = 34 * 16, cb = 70 * 16;
    for (int i = 0; i < 6; i++) {
        Nuvola *c = &A.nv[i]; int w = A.mw[c->m], h = A.mh[c->m]; const uint8_t *m = A.maschera[c->m];
        int nx = (int)lroundf(c->x) - CX, ny = (int)lroundf(c->y) - CY;
        int j0 = clampi(ny / 2, 0, LH), j1 = clampi((ny + h) / 2 + 1, 0, LH), i0 = clampi(nx / 2, 0, LW), i1 = clampi((nx + w) / 2 + 1, 0, LW);
        for (int j = j0; j < j1; j++) { int my = j * 2 - ny; if (my < 0 || my >= h) continue;
            for (int ii = i0; ii < i1; ii++) { int mx = ii * 2 - nx; if (mx < 0 || mx >= w || !m[my * w + mx]) continue;
                int q = j * LW + ii; LR[q] += ((cr - LR[q]) * k) >> 8; LG[q] += ((cg - LG[q]) * k) >> 8; LB[q] += ((cb - LB[q]) * k) >> 8; } }
    }
}
/* bagliore (come postBagliore del PC): le parti più luminose, elevate a potenza, sfocate in due misure e sommate in «screen» */
#define QW (SW / 4)
#define QH (SH / 4)
static uint16_t GQ[3][QW * QH], GW[3][(QW / 2) * (QH / 2)], GH[3][LW * LH];
static void bagliore(Sup *s, float forza, int passi) {
    if (forza <= .01f) return;
    static int turno = 0; static bool pronto = false;
    if (pronto && (++turno & 1)) goto applica;                /* la sfocatura si ricalcola un fotogramma sì e uno no */
    pronto = true;
    for (int j = 0; j < QH; j++) for (int i = 0; i < QW; i++) {             /* media di 4 campioni per cella 4×4, poi potenza */
        int r = 0, g = 0, b = 0;
        for (int k = 0; k < 4; k++) { int c = s->px[(j * 4 + 1 + (k >> 1) * 2) * SW + i * 4 + 1 + (k & 1) * 2]; r += (c >> 11) << 3; g += ((c >> 5) & 63) << 2; b += (c & 31) << 3; }
        int v[3] = { r * 4096 / 1020, g * 4096 / 1020, b * 4096 / 1020 };        /* 0..4096, poi v⁴ (v⁸ di giorno) */
        for (int c = 0; c < 3; c++) { int x = (v[c] * v[c]) >> 12; x = (x * x) >> 12; if (passi > 2) x = (x * x) >> 12; GQ[c][j * QW + i] = (uint16_t)x; }
    }
    for (int c = 0; c < 3; c++) for (int j = 0; j < QH / 2; j++) for (int i = 0; i < QW / 2; i++) {
        const uint16_t *q = GQ[c] + j * 2 * QW + i * 2; GW[c][j * (QW / 2) + i] = (q[0] + q[1] + q[QW] + q[QW + 1]) >> 2;
    }
    /* alla risoluzione della mappa di luce: interpolazione bilineare delle due sfocature (tabelle precalcolate) */
    static struct { uint8_t q0, q1, fq, w0, w1, fw; } TX[LW], TY[LH]; static bool tab = false;
    if (!tab) {
        for (int i = 0; i < LW; i++) { float q = (i + .5f) / 2 - .5f, w = (i + .5f) / 4 - .5f; int a0 = clampi((int)floorf(q), 0, QW - 1), b0 = clampi((int)floorf(w), 0, QW / 2 - 1);
            TX[i].q0 = a0; TX[i].q1 = clampi(a0 + 1, 0, QW - 1); TX[i].fq = (uint8_t)clampi((int)((q - floorf(q)) * 256), 0, 255); TX[i].w0 = b0; TX[i].w1 = clampi(b0 + 1, 0, QW / 2 - 1); TX[i].fw = (uint8_t)clampi((int)((w - floorf(w)) * 256), 0, 255); }
        for (int j = 0; j < LH; j++) { float q = (j + .5f) / 2 - .5f, w = (j + .5f) / 4 - .5f; int a0 = clampi((int)floorf(q), 0, QH - 1), b0 = clampi((int)floorf(w), 0, QH / 2 - 1);
            TY[j].q0 = a0; TY[j].q1 = clampi(a0 + 1, 0, QH - 1); TY[j].fq = (uint8_t)clampi((int)((q - floorf(q)) * 256), 0, 255); TY[j].w0 = b0; TY[j].w1 = clampi(b0 + 1, 0, QH / 2 - 1); TY[j].fw = (uint8_t)clampi((int)((w - floorf(w)) * 256), 0, 255); }
        tab = true;
    }
    int fa = (int)(fminf(1, forza * .75f) * 256), fb = (int)(fminf(1, forza) * 256), WW = QW / 2;
    for (int c = 0; c < 3; c++) {
        const uint16_t *Q = GQ[c], *W = GW[c]; uint16_t *O = GH[c];
        for (int j = 0; j < LH; j++) {
            const uint16_t *qa = Q + TY[j].q0 * QW, *qb = Q + TY[j].q1 * QW, *wa = W + TY[j].w0 * WW, *wb = W + TY[j].w1 * WW; int fqy = TY[j].fq, fwy = TY[j].fw;
            for (int i = 0; i < LW; i++) {
                int x0 = TX[i].q0, x1 = TX[i].q1, fq = TX[i].fq, y0 = TX[i].w0, y1 = TX[i].w1, fw = TX[i].fw;
                int a0 = qa[x0] + (((qa[x1] - qa[x0]) * fq) >> 8), a1 = qb[x0] + (((qb[x1] - qb[x0]) * fq) >> 8), na = a0 + (((a1 - a0) * fqy) >> 8);
                int b0 = wa[y0] + (((wa[y1] - wa[y0]) * fw) >> 8), b1 = wb[y0] + (((wb[y1] - wb[y0]) * fw) >> 8), nb = b0 + (((b1 - b0) * fwy) >> 8);
                int x = (na * fa) >> 8, y = (nb * fb) >> 8;                      /* screen: x + y - x·y */
                *O++ = (uint16_t)(x + y - ((x * y) >> 12));
            }
        }
    }
applica:
    for (int j = 0; j < LH; j++) {
        uint16_t *r0 = s->px + j * 2 * SW, *r1 = r0 + SW;
        for (int i = 0; i < LW; i++) {
            int q = j * LW + i, gr = GH[0][q] >> 4, gg = GH[1][q] >> 4, gb = GH[2][q] >> 4;   /* 0..256 */
            if (gr < 3 && gg < 3 && gb < 3) continue;
            uint16_t *p[4] = { r0 + i * 2, r0 + i * 2 + 1, r1 + i * 2, r1 + i * 2 + 1 };
            for (int k = 0; k < 4; k++) {
                int c = *p[k], R5 = c >> 11, G6 = (c >> 5) & 63, B5 = c & 31;
                R5 += ((31 - R5) * gr) >> 8; G6 += ((63 - G6) * gg) >> 8; B5 += ((31 - B5) * gb) >> 8;   /* screen sul pixel */
                *p[k] = (uint16_t)((R5 << 11) | (G6 << 5) | B5);
            }
        }
    }
}
/* alone scuro intorno a Zambu (personaggio misterioso), sotto a tutto */
static void aura_zambu(Sup *s) {
    if (!ZB.c || ZB.stato == ZB_VIA) return;
    int cx = (int)ZB.x - CX, cy = (int)ZB.y - 4 - CY; float al = ZB.alpha;
    for (int y = clampi(cy - 60, 0, SH); y < clampi(cy + 60, 0, SH); y++) for (int x = clampi(cx - 60, 0, SW); x < clampi(cx + 60, 0, SW); x++) {
        float d = sqrtf((float)((x - cx) * (x - cx) + (y - cy) * (y - cy))) / 60; if (d >= 1) continue;
        float a = d < .5f ? .55f - .5f * d : .3f * (1 - (d - .5f) / .5f);
        uint16_t *p = s->px + y * SW + x; *p = mix565(*p, rgb(6, 10, 9), (int)(a * al * 256));
    }
}
static void illumina(Sup *s) {
    Mappa *m = M_ATT();
    float amb[3] = { S.amb[0], S.amb[1], S.amb[2] }, notte = S.notte;
    if (m->interno) { notte = 0; if (m->sotterraneo) { amb[0] = .25f; amb[1] = .28f; amb[2] = .35f; } else { amb[0] = .97f; amb[1] = .94f; amb[2] = .9f; } }
    int ar = (int)(amb[0] * 4096), ag = (int)(amb[1] * 4096), ab = (int)(amb[2] * 4096);
    for (int i = 0; i < LW * LH; i++) { LR[i] = ar; LG[i] = ag; LB[i] = ab; }
    if (!m->interno) nuvole();
    if (UI.luci && (notte > .02f || m->interno)) {
        if (notte > .02f) luce(S.px, S.py - 14, 34, 150, 170, 230, .14f * notte);   /* un filo di luce lunare sul personaggio */
        for (int i = 0; i < A.nl; i++) { float b = lucciola_luce(&A.l[i]); if (b > .02f) luce(A.l[i].x, A.l[i].y, 12, 200, 255, 140, .5f * b * notte); }
        for (int i = 0; i < m->nluci; i++) {
            Luce *l = &m->luci[i]; float k = l->interna && m->interno ? 1 : notte; if (k < .02f) continue;
            if (ZB.c && ZB.stato != ZB_FERMO && fabsf(l->x - (ZB.tx * T + 8)) < 20 && fabsf(l->y - ZB.ty * T) < 40) continue;   /* la brace di Zambu c'è solo quando c'è lui */
            luce(l->x, l->y, l->r, l->cr, l->cg, l->cb, l->a / 255.f * k);
        }
        if (m->sotterraneo && S.fuori > .05f) for (int i = 0; i < m->ndiurne; i++) luce(m->diurne[i * 2], m->diurne[i * 2 + 1], 70, 255, 236, 200, .5f * S.fuori);
    }
    if (!VIGN_OK) {                                           /* vignettatura: sfumatura radiale verso un blu notte (8,10,28), 0 → 0,85 */
        for (int j = 0; j < LH; j++) for (int i = 0; i < LW; i++) {
            float dx = (i + .5f - LW / 2.f) * 2, dy = (j + .5f - LH / 2.f) * 2, d = sqrtf(dx * dx + dy * dy) / (sqrtf(SW * SW + SH * SH) / 2);
            VIGN[j * LW + i] = (uint8_t)(clampf((d - .35f) / .65f, 0, 1) * .85f * 255);
        }
        VIGN_OK = true;
    }
    int forza = UI.vignetta ? (int)(((m->interno ? .3f : .1f) + .45f * notte) * 256) : 0;
    float oro = UI.effetti ? fminf(1, A.oro) * .07f : 0;            /* ora dorata: un velo caldo (soft-light arancio) */
    int tr = (int)((1 + oro * .9f) * 256), tg = (int)((1 + oro * .25f) * 256), tb = (int)((1 - oro * .7f) * 256);
    /* composizione: ogni cella della mappa di luce copre 2×2 pixel */
    for (int j = 0; j < LH; j++) {
        uint16_t *r0 = s->px + j * 2 * SW, *r1 = r0 + SW;
        for (int i = 0; i < LW; i++) {
            int q = j * LW + i, va = (VIGN[q] * forza) >> 8, vk = 256 - va;
            int mr = LR[q] > 4096 ? 4096 : LR[q], mg = LG[q] > 4096 ? 4096 : LG[q], mb = LB[q] > 4096 ? 4096 : LB[q];
            mr = (((mr * vk) >> 12) * tr) >> 8; mg = (((mg * vk) >> 12) * tg) >> 8; mb = (((mb * vk) >> 12) * tb) >> 8;   /* 0..256 */
            if (mr >= 256 && mg >= 256 && mb >= 256 && va == 0) continue;
            uint16_t *p[4] = { r0 + i * 2, r0 + i * 2 + 1, r1 + i * 2, r1 + i * 2 + 1 };
            for (int k = 0; k < 4; k++) {
                int c = *p[k], R5 = ((c >> 11) * mr + 1 * va) >> 8, G6 = (((c >> 5) & 63) * mg + 2 * va) >> 8, B5 = ((c & 31) * mb + 3 * va) >> 8;   /* + blu notte (8,10,28) */
                if (R5 > 31) R5 = 31; if (G6 > 63) G6 = 63; if (B5 > 31) B5 = 31;
                *p[k] = (uint16_t)((R5 << 11) | (G6 << 5) | B5);
            }
        }
    }
    /* vetri accesi dei lampioni, sopra l'ombra */
    for (int i = 0; i < m->nvetri; i++) {
        Vetro *v = &m->vetri[i]; float k = v->interna && m->interno ? 1 : notte; if (k < .02f || !UI.luci) continue;
        int x = v->x - CX, y = v->y - CY; if (x < -8 || y < -8 || x > SW || y > SH) continue;
        sup_rett_a(s, x, y, v->w, v->h, v->interna ? rgb(244, 248, 255) : rgb(255, 241, 191), (int)(.95f * k * 256));
        if (v->interna) sup_somma(s, x - 2, y - 2, v->w + 4, v->h + 4, (int)(188 * .22f * k), (int)(212 * .22f * k), (int)(255 * .22f * k));
        else sup_somma(s, x - 2, y - 2, v->w + 4, v->h + 4, (int)(255 * .22f * k), (int)(211 * .22f * k), (int)(138 * .22f * k));
    }
    if (!m->interno) atmo_lucciole(s, CX, CY);
    if (UI.bagliore) bagliore(s, m->interno ? .45f : .35f + .95f * notte + .2f * fminf(1, A.oro), !m->interno && notte < .4f ? 3 : 2);
}

void mondo_disegna(Sup *s, bool titolo) {
    Mappa *m = M_ATT(); int MWp = m->W * T, MHp = m->H * T;
    float fx = titolo ? m->spx * T + 8 : S.px, fy = titolo ? m->spy * T : S.py - 16;
    CX = MWp <= SW ? (MWp - SW) / 2 : clampi((int)lroundf(fx) - SW / 2, 0, MWp - SW);
    CY = MHp <= SH ? (MHp - SH) / 2 : clampi((int)lroundf(fy) - SH / 2, 0, MHp - SH);
    S.camx = CX; S.camy = CY;
    if (MWp < SW || MHp < SH) sup_riempi(s, 0);
    terreno(s);
    aura_zambu(s);
    raccoglibili(s);
    scena(s, titolo);
    if (UI.effetti) atmo_mondo(s, CX, CY);
    illumina(s);
    if (S.fade >= 0 && !titolo) { float a = S.fade < .22f ? S.fade / .22f : fmaxf(0, 1 - (S.fade - .22f) / .33f); sup_rett_a(s, 0, 0, SW, SH, 0, (int)(a * 256)); }
}
