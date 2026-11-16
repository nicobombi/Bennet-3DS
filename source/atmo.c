/* atmosfera: stesso comportamento della versione per PC (atmoUpdate, atmoWorld, smokeParts), in C */
#include "atmo.h"
#include "stato.h"

Atmo A;

uint32_t hash3(int a, int b, int s) {
    uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u + (uint32_t)s * 1442695041u;
    h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16);
}
float hashf(int a, int b, int s) { return hash3(a, b, s) / 4294967296.f; }
float ah(int x, int y, int k) { return hashf(x + 11, y + 5, k + 3); }

void atmo_inizia(void) {
    A.dir = 1; A.forza = .45f;
    /* ombre delle nuvole: tre maschere di macchie ovali con il bordo «a puntini» */
    for (int k = 0; k < 3; k++) {
        int w = 110 + k * 22, h = 54 + k * 8; float bl[6][4];
        for (int i = 0; i < 6; i++) { bl[i][0] = w * (.2f + .6f * ah(i, k, 1)); bl[i][1] = h * (.35f + .3f * ah(i, k, 2)); bl[i][2] = w * (.13f + .1f * ah(i, k, 3)); bl[i][3] = h * (.22f + .12f * ah(i, k, 4)); }
        uint8_t *m = calloc(w * h, 1);
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
            float v = 0; for (int i = 0; i < 6; i++) { float a = (x - bl[i][0]) / bl[i][2], b = (y - bl[i][1]) / bl[i][3], q = 1 - a * a - b * b; if (q > v) v = q; }
            m[y * w + x] = v > 0 && v < .25f ? (ah(x, y, 5 + k) < v * 4) : v > 0;
        }
        A.maschera[k] = m; A.mw[k] = w; A.mh[k] = h;
    }
    for (int i = 0; i < 6; i++) { A.nv[i].m = i % 3; A.nv[i].sp = .7f + ah(i, 3, 6) * .6f; }
}
void atmo_entra(float px, float py) {
    A.np = A.ns = A.nl = 0;
    for (int i = 0; i < 6; i++) { A.nv[i].x = px - 420 + ah(i, 1, 6) * 840; A.nv[i].y = py - 260 + ah(i, 2, 6) * 520; }
}

static void nuovo_polline(int cx, int cy) {
    if (A.np >= N_MOTE) return;
    static const uint16_t PET[3] = { 0xF63A, 0xFFFF, 0xF558 }, POL[4] = { 0xFFBA, 0xF734, 0xFFFF, 0xEF99 };
    Polline *p = &A.p[A.np++]; p->petalo = rndf() < .14f;
    p->x = cx - 20 + rndf() * (SW + 40); p->y = cy + rndf() * SH; p->vy = -1 + rndf() * 4; p->drag = .45f + rndf() * .5f;
    p->bob = 4 + rndf() * 8; p->ph = rndf() * 6; p->spin = 1 + rndf() * 2.5f; p->life = 0; p->max = 4 + rndf() * 5;
    p->c = p->petalo ? PET[rnd() % 3] : POL[rnd() % 4];
}
static void nuova_scia(int cx, int cy) {
    if (A.ns >= N_SCIA) return;
    Scia *s = &A.sc[A.ns++]; s->dir = A.dir;
    s->x = A.dir > 0 ? cx + rndf() * SW * .5f : cx + SW * (.5f + rndf() * .5f); s->y = cy + 20 + rndf() * (SH - 40);
    s->len = 70 + rndf() * 80; s->amp = 2 + rndf() * 4; s->fr = .05f + rndf() * .05f; s->ph = rndf() * 6; s->life = 0; s->max = 1.4f + rndf() * .8f; s->ricciolo = rndf() < .45f;
}
static void nuova_lucciola(int cx, int cy) {
    if (A.nl >= N_LUCC) return;
    Mappa *m = M_ATT();
    for (int k = 0; k < 6; k++) {
        float x = cx + rndf() * SW, y = cy + rndf() * SH; int tx = (int)(x / T), ty = (int)(y / T);
        if (tx < 1 || ty < 1 || tx >= m->W - 1 || ty >= m->H - 1 || m->solid[ty * m->W + tx]) continue;
        Lucciola *f = &A.l[A.nl++]; memset(f, 0, sizeof *f); f->x = x; f->y = y; f->ph = rndf() * 6; f->max = 7 + rndf() * 8; f->blink = 1.2f + rndf() * 1.6f; f->off = rndf() * 6;
        return;
    }
}
float lucciola_luce(const Lucciola *f) {
    float fade = fminf(fminf(1, f->life), (f->max - f->life) / 1.2f); if (fade < 0) fade = 0;
    float s = sinf(f->ph * f->blink * 2 + f->off); if (s < 0) s = 0;
    return s * s * fade;
}

void atmo_passo(float dt, int cx, int cy, bool interno, float notte, float ora) {
    A.notte = interno ? 0 : notte; A.giorno = interno ? 0 : 1 - notte;
    A.oro = interno ? 0 : fmaxf(0, 1 - fabsf(ora - 18.2f) / 1.3f) + fmaxf(0, 1 - fabsf(ora - 7.1f) / 1.1f) * .65f;
    /* vento: base + raffiche da somma di sinusoidi lente, ogni tanto un colpo più forte */
    A.wt += dt;
    float g = .55f * sinf(A.wt * .21f) + .3f * sinf(A.wt * .57f + 1.3f) + .15f * sinf(A.wt * 1.31f + .4f);
    float burst = fmaxf(0, sinf(A.wt * .09f + 2) - .82f) * 5;
    A.gust += (clampf(g * .9f + burst, 0, 1) - A.gust) * fminf(1, dt * 1.5f);
    A.forza = .32f + .55f * A.gust;
    A.windT = fmodf(A.windT + dt * .4f * (.6f + .9f * A.gust), 1);
    A.fxT += dt;
    float wx = A.dir * (10 + 34 * A.forza);
    /* nuvole */
    for (int i = 0; i < 6; i++) {
        Nuvola *c = &A.nv[i]; int w = A.mw[c->m], h = A.mh[c->m]; c->x += wx * .35f * c->sp * dt;
        if (c->x > cx + SW + 260 || c->x + w < cx - 520 || c->y > cy + SH + 220 || c->y + h < cy - 220) {
            c->x = A.dir > 0 ? cx - w - 40 - rndf() * 220 : cx + SW + 40 + rndf() * 220; c->y = cy - 60 + rndf() * (SH + 120);
        }
    }
    if (UI.effetti) {
        float area = (float)(SW * SH) / (400 * 260);
        float n = (4 + 9 * A.gust) * A.giorno * area * dt;                 /* pollini e petali di giorno */
        while (n > 0) { if (rndf() < n) nuovo_polline(cx, cy); n -= 1; }
        if (!interno && rndf() < dt * (.25f + 1.4f * A.gust)) nuova_scia(cx, cy);   /* scie di vento */
        int voglio = (int)lroundf(16 * A.notte * area);                    /* lucciole di notte */
        if (A.nl < voglio && rndf() < dt * 6) nuova_lucciola(cx, cy);
    }
    for (int i = 0; i < A.np; i++) {
        Polline *p = &A.p[i]; p->life += dt; p->ph += dt * p->spin;
        p->x += (wx * p->drag + sinf(p->ph) * p->bob) * dt; p->y += (p->vy + cosf(p->ph * .7f) * p->bob * .5f) * dt;
        if (p->life >= p->max) { A.p[i--] = A.p[--A.np]; }
    }
    for (int i = 0; i < A.ns; i++) { A.sc[i].life += dt; if (A.sc[i].life >= A.sc[i].max) A.sc[i--] = A.sc[--A.ns]; }
    for (int i = 0; i < A.nl; i++) {
        Lucciola *f = &A.l[i]; f->life += dt; f->ph += dt;
        f->vx += (rndf() - .5f) * 40 * dt + wx * .02f * dt; f->vy += (rndf() - .5f) * 40 * dt;
        f->vx *= .97f; f->vy *= .97f; f->x += f->vx * dt; f->y += f->vy * dt + sinf(f->ph * 1.7f) * 3 * dt;
        if (A.notte < .05f && f->max - f->life > 1.2f) f->max = f->life + 1.2f;   /* all'alba si spengono piano */
        if (f->life >= f->max) A.l[i--] = A.l[--A.nl];
    }
}

void atmo_mondo(Sup *s, int cx, int cy) {
    for (int i = 0; i < A.np; i++) {
        Polline *p = &A.p[i]; float a = fminf(fminf(1, p->life / .8f), (p->max - p->life) / 1); if (a <= 0) continue;
        int x = (int)lroundf(p->x) - cx, y = (int)lroundf(p->y) - cy, al = (int)(a * .9f * 256);
        if (p->petalo) { if (sinf(p->ph * 2) > 0) sup_rett_a(s, x, y, 2, 1, p->c, al); else sup_rett_a(s, x, y, 1, 2, p->c, al); }
        else sup_rett_a(s, x, y, 1, 1, p->c, al);
    }
    for (int i = 0; i < A.ns; i++) {
        Scia *q = &A.sc[i]; float t = q->life / q->max, head = t * (q->len + 40), tail = head - 26, a = sinf(3.14159f * t) * (.35f + .25f * A.giorno);
        int al = (int)(a * 256); if (al <= 0) continue;
        for (int d = (int)fmaxf(0, tail); d < fminf(q->len, head); d++) {
            float px = q->x + d * q->dir, py = q->y + sinf(d * q->fr + q->ph) * q->amp;
            if (q->ricciolo && d > q->len - 18) {
                float k = (d - (q->len - 18)) / 18 * 3.14159f * 1.6f;
                px = q->x + (q->len - 18) * q->dir + sinf(k) * 7 * q->dir; py = q->y + sinf((q->len - 18) * q->fr + q->ph) * q->amp - (1 - cosf(k)) * 5;
            }
            sup_rett_a(s, (int)lroundf(px) - cx, (int)lroundf(py) - cy, 1, 1, 0xFFFF, al);
        }
    }
}
void atmo_lucciole(Sup *s, int cx, int cy) {
    for (int i = 0; i < A.nl; i++) {
        float b = lucciola_luce(&A.l[i]) * A.notte; if (b < .05f) continue;
        int x = (int)lroundf(A.l[i].x) - cx, y = (int)lroundf(A.l[i].y) - cy;
        sup_rett_a(s, x - 1, y, 3, 1, rgb(200, 255, 138), (int)(b * .45f * 256)); sup_rett_a(s, x, y - 1, 1, 3, rgb(200, 255, 138), (int)(b * .45f * 256));
        sup_rett_a(s, x, y, 1, 1, rgb(246, 255, 184), (int)(b * 256));
    }
}

/* fumo: un filo sottile e ondulato più, ogni tanto, una boccata che si allarga (come smokeParts del PC) */
static void blob(Sup *s, float x, float y, float sz, uint16_t c, int a) {
    int X = (int)lroundf(x), Y = (int)lroundf(y), S2 = (int)lroundf(sz); if (S2 < 1) S2 = 1;
    if (S2 == 1) { sup_rett_a(s, X, Y, 1, 1, c, a); return; }
    sup_rett_a(s, X - S2 + 1, Y - S2, 2 * S2 - 2, 2 * S2, c, a); sup_rett_a(s, X - S2, Y - S2 + 1, 1, 2 * S2 - 2, c, a); sup_rett_a(s, X + S2 - 1, Y - S2 + 1, 1, 2 * S2 - 2, c, a);
}
typedef struct { float x, y, s, al; } Sbuffo;
void fumo_disegna(Sup *s, int cx, int cy, float ox, float oy, int sd0, int denso, float t) {
    float L = denso ? 3.8f : 3.4f, C = denso ? 2.6f : 5.4f, rise = denso ? 1.25f : 1; int N = denso ? 22 : 14, puffs = denso ? 9 : 6;
    uint16_t col = denso ? rgb(201, 203, 205) : rgb(223, 225, 228);
    int r = (col >> 11) << 3, g = ((col >> 5) & 63) << 2, b = (col & 31) << 3;
    uint16_t lo = rgb(r * 62 / 100, g * 62 / 100, b * 62 / 100), hi = rgb(r * 112 / 100 > 255 ? 255 : r * 112 / 100, g * 112 / 100 > 255 ? 255 : g * 112 / 100, b * 112 / 100 > 255 ? 255 : b * 112 / 100);
    float sd = (float)(((sd0 * 9301 + (int)(ox / T) * 131 + (int)(oy / T) * 71) % 997 + 997) % 997) / 97;
    Sbuffo P[48]; int n = 0;
    for (int i = 0; i < N; i++) {
        float a = fmodf(t + i * L / N + sd, L) / L, age = a * L;
        P[n++] = (Sbuffo){ ox + sinf(age * 2.6f + sd + i * .7f) * (.6f + a * 3.5f) + a * a * 9, oy - age * 11 * rise - a * a * 8, .8f + a * 1.8f, fminf(1, a / .06f) * (1 - a) * .72f };
    }
    for (int back = 0; back < 2; back++) {
        int k = (int)floorf((t + sd) / C) - back; float t0 = k * C - sd; int dir = hashf(k, (int)(ox / T), (int)(oy / T) + 3) < .5f ? -1 : 1;
        for (int j = 0; j < puffs && n < 48; j++) {
            float age = t - (t0 + j * .08f); if (age < 0 || age > 4.2f) continue;
            float sp = .6f + hashf(k, j, (int)(ox / T)) * .8f, a = age / 4.2f;
            P[n++] = (Sbuffo){ ox + (1 - expf(-age * 2.2f)) * (2 + j * .9f) * dir * sp + sinf(age * 1.7f + j) * age * 1.3f + age * age * .9f,
                oy - (1 - expf(-age * 1.8f)) * (5 + j) * rise - age * 9 * rise, 1.2f + age * 1.25f + j * .15f, fminf(1, age / .12f) * powf(1 - a, 1.4f) * .85f };
        }
    }
    for (int i = 1; i < n; i++) for (int j = i; j > 0 && P[j].y > P[j - 1].y; j--) { Sbuffo q = P[j]; P[j] = P[j - 1]; P[j - 1] = q; }
    for (int i = 0; i < n; i++) {
        Sbuffo *p = &P[i]; if (p->al <= .01f) continue;
        float x = p->x - cx, y = p->y - cy;
        blob(s, x + .6f, y + .8f, p->s, lo, (int)(p->al * .55f * 256));
        blob(s, x, y, p->s, col, (int)(p->al * 256));
        if (p->s >= 2) blob(s, x - p->s * .3f, y - p->s * .35f, p->s * .5f, hi, (int)(p->al * .8f * 256));
    }
}
