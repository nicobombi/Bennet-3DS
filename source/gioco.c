/* logica del gioco: personaggio, tempo, passaggi, Siro, Zambu, Andrea, dialoghi, inventario, salvataggi */
#include "stato.h"
#include "dati.h"
#include "atmo.h"
#include <stdarg.h>

Stato S; Zambu ZB; Siro SI; Passeggio PS; Dialogo DLG; Negozio NEG; Toast TOAST[3]; Ui UI; Brillo BRILLI[4];
char PERCORSO_SALVA[256] = "salvataggio.bin";
static bool ESCI = false;

static int IT_SHOTA = -1, IT_ORO = -1;
Mappa *M_ATT(void) { return &MAPPE[S.mappa]; }
float ora_del_giorno(void) { float h = fmodf(S.minuti / 60.f, 24.f); return h < 0 ? h + 24 : h; }
static float frand(void) { return rndf(); }

void toast(const char *fmt, ...) {
    Toast *t = &TOAST[0];
    for (int i = 0; i < 3; i++) if (TOAST[i].vita <= 0) { t = &TOAST[i]; goto ok; }
    for (int i = 1; i < 3; i++) if (TOAST[i].vita < t->vita) t = &TOAST[i];
ok:;
    va_list a; va_start(a, fmt); vsnprintf(t->t, sizeof t->t, fmt, a); va_end(a); t->vita = 2.8f;
}
/* la «r moscia» di Siro: ogni r diventa v */
const char *moscia(const char *s, char *b, int cap) {
    int i = 0; for (; s[i] && i < cap - 1; i++) b[i] = s[i] == 'r' ? 'v' : s[i] == 'R' ? 'V' : s[i];
    b[i] = 0; return b;
}

/* ---------------- inventario ---------------- */
int item_conta(int item) { int n = 0; for (int i = 0; i < NSLOT; i++) if (S.inv[i].item == item) n += S.inv[i].n; return n; }
bool item_dai(int item, int n) {
    if (item < 0) return false;
    int max = ITEMS[item].max > 0 ? ITEMS[item].max : 99, posto = 0;
    for (int i = 0; i < NSLOT; i++) posto += S.inv[i].item == item ? max - S.inv[i].n : S.inv[i].item < 0 ? max : 0;
    if (posto < n) return false;
    for (int i = 0; i < NSLOT && n; i++) if (S.inv[i].item == item && S.inv[i].n < max) { int k = max - S.inv[i].n; if (k > n) k = n; S.inv[i].n += k; n -= k; }
    for (int i = 0; i < NSLOT && n; i++) if (S.inv[i].item < 0) { int k = n > max ? max : n; S.inv[i].item = item; S.inv[i].n = k; n -= k; }
    return true;
}
bool item_togli(int item) {
    for (int i = NSLOT - 1; i >= 0; i--) if (S.inv[i].item == item) { if (--S.inv[i].n <= 0) { S.inv[i].item = -1; S.inv[i].n = 0; } return true; }
    return false;
}

/* ---------------- collisioni ---------------- */
static bool solido(int tx, int ty) { Mappa *m = M_ATT(); if (tx < 0 || ty < 0 || tx >= m->W || ty >= m->H) return true; return m->solid[ty * m->W + tx]; }
static bool bloccato(float x, float y) {
    int x0 = (int)floorf((x - 5) / T), x1 = (int)floorf((x + 5) / T), y0 = (int)floorf((y - 5) / T), y1 = (int)floorf((y - .01f) / T);
    for (int j = y0; j <= y1; j++) for (int i = x0; i <= x1; i++) if (solido(i, j)) return true;
    return false;
}
static int passo_in(int tx, int ty) {
    Mappa *m = M_ATT();
    for (int i = 0; i < m->npassi; i++) { Passo *p = &m->passi[i]; if (tx >= p->x && tx < p->x + p->w && ty >= p->y && ty < p->y + p->h) return i; }
    return -1;
}

/* ---------------- Zambu ---------------- */
static bool zb_presente(void) { float m = fmodf(S.minuti, 1440.f); return m >= 22 * 60 + 30 || m < 6 * 60; }
static void zb_init(void) {
    ZB.L = 0; ZB.seg[0] = 0;
    for (int i = 0; i < NZB; i++) { ZB.pts[i][0] = (ZB.tx + ZB_PERCORSO[i][0]) * T + 8; ZB.pts[i][1] = (ZB.ty + 1 + ZB_PERCORSO[i][1]) * T; }
    for (int i = 1; i < NZB; i++) ZB.seg[i] = ZB.seg[i - 1] + hypotf(ZB.pts[i][0] - ZB.pts[i - 1][0], ZB.pts[i][1] - ZB.pts[i - 1][1]);
    ZB.L = ZB.seg[NZB - 1];
    bool c = zb_presente(); ZB.s = c ? ZB.L : 0; ZB.stato = c ? ZB_FERMO : ZB_VIA; ZB.dir = D_GIU; ZB.passo = 0;
    ZB.x = ZB.pts[NZB - 1][0]; ZB.y = ZB.pts[NZB - 1][1]; ZB.alpha = c ? 1 : 0;
}
static void zb_aggiorna(float dt) {
    if (!ZB.c) return;
    bool pres = zb_presente();
    if (pres && ZB.stato != ZB_FERMO) ZB.stato = ZB_ARRIVA;
    if (!pres && ZB.stato != ZB_VIA) ZB.stato = ZB_PARTE;
    if (ZB.stato == ZB_ARRIVA) { ZB.s += 24 * dt; if (ZB.s >= ZB.L) { ZB.s = ZB.L; ZB.stato = ZB_FERMO; } }
    else if (ZB.stato == ZB_PARTE) { ZB.s -= 24 * dt; if (ZB.s <= 0) { ZB.s = 0; ZB.stato = ZB_VIA; } }
    float x = ZB.pts[NZB - 1][0], y = ZB.pts[NZB - 1][1], dx = 0, dy = 0;
    for (int i = 1; i < NZB; i++) if (ZB.s <= ZB.seg[i] || i == NZB - 1) {
        float k = clampf((ZB.s - ZB.seg[i - 1]) / ((ZB.seg[i] - ZB.seg[i - 1]) ? (ZB.seg[i] - ZB.seg[i - 1]) : 1), 0, 1);
        dx = ZB.pts[i][0] - ZB.pts[i - 1][0]; dy = ZB.pts[i][1] - ZB.pts[i - 1][1]; x = ZB.pts[i - 1][0] + dx * k; y = ZB.pts[i - 1][1] + dy * k; break;
    }
    ZB.x = x; ZB.y = y; ZB.alpha = fminf(1, ZB.s / 16);
    if (ZB.stato == ZB_ARRIVA || ZB.stato == ZB_PARTE) {
        float sg = ZB.stato == ZB_ARRIVA ? 1 : -1, mx = dx * sg, my = dy * sg;
        ZB.dir = fabsf(mx) > fabsf(my) ? (mx > 0 ? D_DX : D_SX) : (my > 0 ? D_GIU : D_SU); ZB.passo += dt;
    } else { ZB.dir = D_GIU; ZB.passo = 0; }
    Mappa *m = M_ATT(); int i = ZB.ty * m->W + ZB.tx; bool sopra = fabsf(S.px - x) < 14 && fabsf(S.py - y) < 14;
    if (ZB.stato == ZB_FERMO) { if (!sopra) m->solid[i] = 1; } else m->solid[i] = 0;
    if (ZB.stato == ZB_VIA && (DLG.aperto || NEG.aperto) && DLG.chi == NPC_ZAMBU) { DLG.aperto = false; NEG.aperto = false; }
}

/* ---------------- Andrea e il barboncino (solo nel parco, dalle 7 alle 21) ---------------- */
static bool ps_orario(void) { float m = fmodf(S.minuti, 1440.f); return m >= 7 * 60 && m < 21 * 60; }
static bool ps_strada(int x, int y) {
    Mappa *m = M_ATT(); if (x < 0 || y < 0 || x >= m->W || y >= m->H) return false;
    int t = m->ter[y * m->W + x]; if (m->solid[y * m->W + x]) return false;
    return t == 1 || t == 6 || t == 7 || t == 9 || t == 51 || t == 50 || (t >= 37 && t <= 45);
}
static bool ps_visibile(float x, float y) { return x > S.camx - 24 && x < S.camx + SW + 24 && y > S.camy - 40 && y < S.camy + SH + 24; }
static int ps_diri(float dx, float dy, int prima) { return fabsf(dx) > fabsf(dy) ? (dx > 0 ? D_DX : D_SX) : fabsf(dy) > 0 ? (dy > 0 ? D_GIU : D_SU) : prima; }
static void ps_comparsa(void) {
    Mappa *m = M_ATT(); int px = -1, py = -1;
    for (int i = 0; i < 6000 && px < 0; i++) {
        int x = rnd() % m->W, y = rnd() % m->H; if (!ps_strada(x, y)) continue;
        float wx = x * T + 8, wy = y * T + 13; if (ps_visibile(wx, wy) || hypotf(wx - S.px, wy - S.py) < 14 * T) continue; px = x; py = y;
    }
    for (int i = 0; i < 20000 && px < 0; i++) { int x = rnd() % m->W, y = rnd() % m->H; if (ps_strada(x, y)) { px = x; py = y; } }
    if (px < 0) { PS.ok = false; return; }
    PS.tx = px; PS.ty = py; PS.x = px * T + 8; PS.y = py * T + 13; PS.dx = PS.dy = 0; PS.pausa = 1 + frand() * 2; PS.nscia = 0;
    PS.cane.x = PS.x - 10; PS.cane.y = PS.y + 2; PS.cane.dir = D_DX; PS.cane.fermo = true; PS.ok = true; PS.vel = 22;
}
static void ps_prossima(void) {
    static const int D[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
    float w[4]; int n = 0; float tot = 0;
    for (int k = 0; k < 4; k++) {
        int nx = PS.tx + D[k][0], ny = PS.ty + D[k][1]; w[k] = 0; if (!ps_strada(nx, ny)) continue;
        float p = (D[k][0] == PS.dx && D[k][1] == PS.dy) ? 9 : (D[k][0] == -PS.dx && D[k][1] == -PS.dy && (PS.dx || PS.dy)) ? .15f : 2.2f;
        int vic = 0; for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) if (ps_strada(nx + i, ny + j)) vic++;
        w[k] = p * (.3f + vic / 9.f); tot += w[k]; n++;
    }
    if (!n) { PS.dx = PS.dy = 0; PS.pausa = 2; return; }
    float r = frand() * tot; int s = 0;
    for (int k = 0; k < 4; k++) { if (w[k] <= 0) continue; s = k; r -= w[k]; if (r <= 0) break; }
    PS.dx = D[s][0]; PS.dy = D[s][1]; PS.tx += PS.dx; PS.ty += PS.dy;
    if (frand() < .035f) PS.pausa = 2 + frand() * 3.5f;
}
static void ps_aggiorna(float dt) {
    if (S.mappa != 0) { PS.ok = false; return; }
    if (!PS.ok) { if (ps_orario() && frand() < dt * .5f) ps_comparsa(); if (!PS.ok) return; }
    if (!ps_orario() && !ps_visibile(PS.x, PS.y) && !PS.parla) { PS.ok = false; return; }
    if (PS.parla && !DLG.aperto) PS.parla = false;
    Cane *C = &PS.cane; bool mosso = false;
    if (PS.parla) PS.dir = ps_diri(S.px - PS.x, S.py - PS.y, PS.dir);
    else if (PS.pausa > 0) PS.pausa -= dt;
    else {
        float gx = PS.tx * T + 8, gy = PS.ty * T + 13, vx = gx - PS.x, vy = gy - PS.y, d = hypotf(vx, vy);
        bool davanti = hypotf(S.px - (PS.x + PS.dx * 10), S.py - (PS.y + PS.dy * 10)) < 11;
        if (davanti) PS.pausa = .6f;
        else if (d < .5f) { PS.x = gx; PS.y = gy; ps_prossima(); }
        else { float s = fminf(d, PS.vel * dt); PS.x += vx / d * s; PS.y += vy / d * s; PS.dist += s; PS.dir = ps_diri(vx, vy, PS.dir); mosso = true; }
    }
    if (mosso) { memmove(PS.scia[1], PS.scia[0], sizeof(PS.scia[0]) * 89); PS.scia[0][0] = PS.x; PS.scia[0][1] = PS.y; if (PS.nscia < 90) PS.nscia++; }
    float tx = C->x, ty = C->y;
    if (PS.nscia > 2) {
        float L = 0; int k = 1;
        for (; k < PS.nscia; k++) { L += hypotf(PS.scia[k][0] - PS.scia[k - 1][0], PS.scia[k][1] - PS.scia[k - 1][1]); if (L >= 12) break; }
        if (k > PS.nscia - 1) k = PS.nscia - 1;
        float sx = PS.scia[k][0], sy = PS.scia[k][1], ax = PS.x - sx, ay = PS.y - sy, al = hypotf(ax, ay); if (al == 0) al = 1;
        tx = sx + (-ay / al) * 5; ty = sy + (ax / al) * 3 + 1;
    }
    float vx = tx - C->x, vy = ty - C->y, d = hypotf(vx, vy), lontano = hypotf(C->x - PS.x, C->y - PS.y);
    if (d > .4f && (mosso || lontano > 16)) {
        float s = fminf(d, (PS.vel + 6) * dt * (d > 6 ? 1.6f : 1)); C->x += vx / d * s; C->y += vy / d * s; C->dist += s; C->dir = ps_diri(vx, vy, C->dir); C->fermo = false;
    } else {
        C->fermo = true; C->guarda -= dt;
        if (C->guarda <= 0) {
            C->guarda = 1.2f + frand() * 2.5f;
            int r = rnd() % 5; C->dir = PS.parla ? ps_diri(S.px - C->x, S.py - C->y, C->dir) : r < 4 ? (int[]){ D_SX, D_DX, D_GIU, D_SU }[r] : ps_diri(PS.x - C->x, PS.y - C->y, C->dir);
        }
    }
    if (PS.cuori > 0) PS.cuori -= dt;
}

/* ---------------- cambio mappa ---------------- */
static void entra(int mappa, int ax, int ay, bool spawn) {
    S.mappa = mappa; Mappa *m = M_ATT();
    if (spawn) { ax = m->spx; ay = m->spy; }
    S.px = ax * T + T / 2; S.py = ay * T + T - 2;
    if (bloccato(S.px, S.py)) {
        for (int r = 1; r < 8; r++) for (int j = -r; j <= r; j++) for (int i = -r; i <= r; i++) {
            float x = (ax + i) * T + T / 2, y = (ay + j) * T + T - 2; if (!bloccato(x, y)) { S.px = x; S.py = y; goto trovato; }
        }
    }
trovato:
    S.passo_dentro = passo_in((int)floorf(S.px / T), (int)floorf((S.py - 3) / T));
    SI.c = ZB.c = false;
    for (int i = 0; i < m->nnpc; i++) {
        if (m->npc[i].k == NPC_SIRO) { SI.c = true; SI.tx = m->npc[i].x; SI.ty = m->npc[i].y; }
        if (m->npc[i].k == NPC_ZAMBU) { ZB.c = true; ZB.tx = m->npc[i].x; ZB.ty = m->npc[i].y; }
    }
    if (ZB.c) zb_init();
    PS.ok = false; PS.parla = false;
    atmo_entra(S.px, S.py);
    S.nome_luogo = 3; S.amb_ok = false; S.moving = false;
    mondo_entra();
    if (m->sotterraneo && S.q_stato == Q_ATTIVA && S.q_passo < 1) { S.q_passo = 1; toast("Quest aggiornata: %s", Q_TITOLO); }
}

/* ---------------- salvataggi ---------------- */
#define SALVA_VER 1
typedef struct {
    char magia[4]; int ver; int mappa; float px, py; int dir; float minuti; int giorno, monete;
    Slot inv[NSLOT]; int mano, q_stato, q_passo; uint8_t raccolti[8]; int8_t mag[16]; int nmag; uint8_t msg_letto;
} Salva;
void partita_salva(void) {
    Salva v; memset(&v, 0, sizeof v); memcpy(v.magia, "VDG3", 4); v.ver = SALVA_VER;
    v.mappa = S.mappa; v.px = S.px; v.py = S.py; v.dir = S.dir; v.minuti = S.minuti; v.giorno = S.giorno; v.monete = S.monete;
    memcpy(v.inv, S.inv, sizeof v.inv); v.mano = S.mano; v.q_stato = S.q_stato; v.q_passo = S.q_passo;
    memcpy(v.raccolti, S.raccolti, 8); memcpy(v.mag, S.mag, 16); v.nmag = S.nmag; v.msg_letto = S.msg_letto;
    char tmp[300]; snprintf(tmp, sizeof tmp, "%s.tmp", PERCORSO_SALVA);
    FILE *f = fopen(tmp, "wb"); if (!f) { toast("Impossibile salvare sulla scheda SD"); return; }
    bool ok = fwrite(&v, sizeof v, 1, f) == 1; ok = fclose(f) == 0 && ok;
    if (!ok) { remove(tmp); toast("Impossibile salvare sulla scheda SD"); return; }
    remove(PERCORSO_SALVA); rename(tmp, PERCORSO_SALVA);          /* prima il file nuovo completo, poi lo scambio: niente salvataggi a metà */
    S.c_salvataggio = true;
}
static bool salva_leggi(Salva *v) {
    FILE *f = fopen(PERCORSO_SALVA, "rb"); if (!f) return false;
    bool ok = fread(v, sizeof *v, 1, f) == 1; fclose(f);
    return ok && !memcmp(v->magia, "VDG3", 4) && v->ver == SALVA_VER && v->mappa >= 0 && v->mappa < NMAPPE;
}
static void stato_base(void) {
    memset(&S.inv, 0, sizeof S.inv); for (int i = 0; i < NSLOT; i++) S.inv[i].item = -1;
    S.mano = 0; S.q_stato = Q_NO; S.q_passo = 0; memset(S.raccolti, 0, 8); S.nmag = 0; S.msg_letto = false;
    S.minuti = 6 * 60; S.giorno = 1; S.monete = 200; S.dir = D_GIU; S.festa = 99; S.fade = -1;
    DLG.aperto = NEG.aperto = false; UI.tab = TAB_ZAINO; UI.sel = -1;
    for (int i = 0; i < 3; i++) TOAST[i].vita = 0;
}
void partita_nuova(void) {
    stato_base(); entra(0, 0, 0, true); S.schermata = SCH_GIOCO; UI.tab = TAB_MSG;
    toast("Benvenuto nella Valle dei Germogli!");
    partita_salva();
}
bool partita_carica(void) {
    Salva v; if (!salva_leggi(&v)) return false;
    stato_base();
    S.minuti = v.minuti; S.giorno = v.giorno; S.monete = v.monete; memcpy(S.inv, v.inv, sizeof S.inv); S.mano = clampi(v.mano, 0, NBARRA - 1);
    S.q_stato = v.q_stato; S.q_passo = v.q_passo; memcpy(S.raccolti, v.raccolti, 8); memcpy(S.mag, v.mag, 16); S.nmag = clampi(v.nmag, 0, 16); S.msg_letto = v.msg_letto;
    for (int i = 0; i < NSLOT; i++) if (S.inv[i].item >= R.nitem || S.inv[i].n == 0) { S.inv[i].item = -1; S.inv[i].n = 0; }
    entra(v.mappa, 0, 0, false); S.px = v.px; S.py = v.py; S.dir = v.dir & 3;
    S.passo_dentro = passo_in((int)floorf(S.px / T), (int)floorf((S.py - 3) / T));
    S.schermata = SCH_GIOCO; UI.tab = TAB_ZAINO;
    return true;
}
void gioco_salva_ora(void) { if (S.schermata == SCH_GIOCO) partita_salva(); }
bool gioco_esci(void) { return ESCI; }
void gioco_chiedi_uscita(void) { partita_salva(); ESCI = true; }

/* ---------------- dialoghi ---------------- */
static void dlg_apri(int chi, const char *nome) { memset(&DLG, 0, sizeof DLG); DLG.aperto = true; DLG.chi = chi; snprintf(DLG.nome, sizeof DLG.nome, "%s", nome); NEG.aperto = false; }
static void dlg_pag(const char *t, bool mosc) { if (DLG.npag >= DLG_PAGINE) return; if (mosc) moscia(t, DLG.pag[DLG.npag], 320); else snprintf(DLG.pag[DLG.npag], 320, "%s", t); DLG.npag++; }
static void dlg_sc(const char *t, int a) { if (DLG.nsc >= 4) return; snprintf(DLG.sc[DLG.nsc].t, 40, "%s", t); DLG.sc[DLG.nsc].a = a; DLG.nsc++; }
static int lung_utf8(const char *s) { int n = 0; for (; *s; s++) if ((*s & 0xC0) != 0x80) n++; return n; }
static bool dlg_finito(void) { return DLG.n >= lung_utf8(DLG.pag[DLG.i]); }
void dialogo_avanza(void) {
    if (!DLG.aperto) return;
    if (!dlg_finito()) { DLG.n = 9999; return; }
    if (DLG.i < DLG.npag - 1) { DLG.i++; DLG.n = 0; DLG.t = 0; }
}
#define SIRO(t) dlg_pag(t, true)

static void siro_parla(void) {
    dlg_apri(NPC_SIRO, "Siro");
    if (S.q_stato == Q_FATTA) {
        static const char *F[] = { "Bro! Guarda: la Shota sta benissimo. Non la tiro più fuori vicino alle grate, giuro.", "Ogni tanto ci penso: senza di te sarei ancora inginocchiato lì sopra.",
            "Tienila bene, la Shota Dorata. È in buone mani, lo sento.", "Ehi, eroe del parcheggio! Tutto bene?" };
        SIRO(F[rnd() % 4]); dlg_sc("Ciao Siro", A_CHIUDI); return;
    }
    if (item_conta(IT_SHOTA) > 0) {
        SIRO("Aspetta... aspetta aspetta aspetta. Cos'è quella cosa che ti spunta dalla tasca?");
        SIRO("...È LEI. È la mia Shota! Bro, l'hai trovata davvero!");
        SIRO("Dov'era? Giù nel parcheggio, vicino all'uscita?! Sarà caduta dalle grate e rotolata fin laggiù... e io qui a fissare il buio per ore come un cretino.");
        SIRO("Fammela vedere. Sì... sì, è proprio lei. Il filtro verde, la carta un po' storta. Non le è successo niente.");
        dlg_sc("Ecco, è tua", A_SIRO_SCAMBIO); dlg_sc("Prima dimmi cos'ha di speciale", A_SIRO_SPECIALE); return;
    }
    bool venduta = false; for (int i = 0; i < S.nmag; i++) if (S.mag[i] == IT_SHOTA) venduta = true;
    if (S.q_stato == Q_ATTIVA && venduta) {
        SIRO("Bro... un tizio strano di notte va in giro a dire che ha una Shota col filtro verde. Dimmi che non gliel'hai venduta.");
        SIRO("Se è così... riprendila. Ti prego. Costi quel che costi.");
        dlg_sc("Rimedio subito", A_CHIUDI); return;
    }
    if (S.q_stato == Q_ATTIVA) {
        static const char *F[] = { "Allora? L'hai trovata? ...No? Ok. Respira, Siro. Respira.", "Il passaggio per scendere dev'essere da qualche parte qui intorno al parcheggio. Credo. Spero.",
            "Ogni minuto che passa là sotto è un minuto di troppo, bro.", "Io resto qui di guardia. Se la sento muoversi ti chiamo." };
        SIRO(F[rnd() % 4]); dlg_sc("Ci sto lavorando", A_CHIUDI); return;
    }
    SIRO("Bro... bro, aspetta. Fermati un attimo, ti prego.");
    SIRO("Mi è caduta la Shota. Giù, tra le grate. Proprio qui sotto, la vedi? ...No, vabbè, non si vede niente.");
    SIRO("Ero qui tranquillo, la tiro fuori per guardarla un secondo e... plin. Scivolata tra le sbarre.");
    SIRO("Ho provato a infilare il braccio ma arrivo fino al gomito e basta. Sotto è buio pesto, si sente solo l'acqua che gocciola.");
    SIRO("Dicono che sotto il parcheggio ci sia un passaggio che porta ai condotti. Io là sotto da solo non ci scendo, giuro.");
    SIRO("Senti... tu mi sembri uno che non ha paura di niente. Ci andresti giù a recuperarmela?");
    dlg_sc("Certo, ci penso io", A_SIRO_SI); dlg_sc("Adesso non posso", A_SIRO_NO);
}
static void zambu_parla(void) {
    dlg_apri(NPC_ZAMBU, "Zambu");
    dlg_pag(FRASI_ZAMBU[rnd() % NFRASI_ZAMBU], false);
    if (S.nmag) dlg_sc("Compra", A_COMPRA);
    dlg_sc("Vendi", A_VENDI); dlg_sc("Arrivederci", A_CHIUDI);
}
static void andrea_parla(void) {
    PS.parla = true; PS.dir = ps_diri(S.px - PS.x, S.py - PS.y, PS.dir);
    dlg_apri(NPC_ANDREA, "Andrea");
    dlg_pag(FRASI_ANDREA[rnd() % NFRASI_ANDREA], false);
    dlg_sc("Accarezza il barboncino", A_ACCAREZZA); dlg_sc("Ciao Andrea", A_CHIUDI);
}
void azione(int a) {
    switch (a) {
    case A_CHIUDI: DLG.aperto = false; break;
    case A_COMPRA: negozio_apri(false); break;
    case A_VENDI: negozio_apri(true); break;
    case A_SIRO_SI:
        if (S.q_stato == Q_NO) { S.q_stato = Q_ATTIVA; S.q_passo = 0; toast("Nuova quest: %s", Q_TITOLO); }
        dlg_apri(NPC_SIRO, "Siro");
        SIRO("Davvero? Bro, sei un grande. Sei veramente un grande.");
        SIRO("Fai attenzione là sotto, è pieno di roba strana. E se la trovi... non guardarla troppo, ok? Riportamela e basta.");
        dlg_sc("Torno presto", A_CHIUDI); partita_salva(); break;
    case A_SIRO_NO:
        dlg_apri(NPC_SIRO, "Siro"); SIRO("Ok... ok, capisco. Io resto qui. Non me ne vado finché non la riprendo."); dlg_sc("Ciao Siro", A_CHIUDI); break;
    case A_SIRO_SPECIALE:
        dlg_apri(NPC_SIRO, "Siro");
        SIRO("Speciale? Bro. L'ho rollata la sera che ho visto le lucciole sul lago per la prima volta.");
        SIRO("Non è una Shota. È un ricordo. E i ricordi non si lasciano in fondo a un parcheggio.");
        dlg_sc("Ecco, è tua", A_SIRO_SCAMBIO); break;
    case A_SIRO_SCAMBIO:
        item_togli(IT_SHOTA);
        if (!item_dai(IT_ORO, 1)) { item_dai(IT_SHOTA, 1); dlg_apri(NPC_SIRO, "Siro"); SIRO("Bro, hai lo zaino pieno: libera un posto, che ti devo dare una cosa importante."); dlg_sc("Ok", A_CHIUDI); break; }
        dlg_apri(NPC_SIRO, "Siro");
        SIRO("Non so nemmeno come ringraziarti. Anzi... sì che lo so.");
        SIRO("Tieni. Questa è la mia Shota Dorata. Me l'ha lasciata mio nonno, e non l'ho mai fatta vedere a nessuno.");
        SIRO("Brilla anche al buio, vedi? Dicono che porti fortuna a chi ha il cuore buono. E tu ce l'hai, bro. Si vede.");
        SIRO("Promettimi solo una cosa: se mai la vendi a quel tipo strano che gira di notte... almeno fatti dare un sacco di monete.");
        dlg_sc("Grazie, Siro!", A_SIRO_FINE); break;
    case A_SIRO_FINE:
        DLG.aperto = false; S.q_stato = Q_FATTA; S.q_passo = NQPASSI; S.festa = 0;
        toast("Quest completata: %s", Q_TITOLO); toast("Hai ricevuto: Shota Dorata!"); partita_salva(); break;
    case A_ACCAREZZA:
        DLG.aperto = false; PS.cuori = 1.8f; PS.cane.dir = ps_diri(S.px - PS.cane.x, S.py - PS.cane.y, PS.cane.dir); PS.cane.guarda = 2;
        toast("Il barboncino scodinzola felice"); break;
    }
}

/* ---------------- bancarella di Zambu ---------------- */
void negozio_apri(bool vendita) {
    DLG.aperto = false; NEG.aperto = true; NEG.vendita = vendita; NEG.sel = 0; NEG.nrighe = 0; DLG.chi = NPC_ZAMBU;
    if (vendita) {
        if (item_conta(IT_SHOTA)) { NEG.righe[NEG.nrighe] = IT_SHOTA; NEG.prezzi[NEG.nrighe++] = ACQ_SHOTA; }
        if (item_conta(IT_ORO)) { NEG.righe[NEG.nrighe] = IT_ORO; NEG.prezzi[NEG.nrighe++] = ACQ_ORO; }
        snprintf(NEG.testa, sizeof NEG.testa, "%s", NEG.nrighe ? "Fammi vedere cos'hai. Pago in oro, subito." : "Non hai niente che mi interessi. Torna quando avrai qualcosa di raro.");
    } else {
        for (int i = 0; i < S.nmag; i++) { NEG.righe[NEG.nrighe] = S.mag[i]; NEG.prezzi[NEG.nrighe++] = S.mag[i] == IT_ORO ? RIV_ORO : S.mag[i] == IT_SHOTA ? RIV_SHOTA : 100; }
        snprintf(NEG.testa, sizeof NEG.testa, "%s", NEG.nrighe ? "Roba buona, trovata in giro. Pagamento in oro." : "Stanotte non ho niente da vendere. Ripassa un'altra notte.");
    }
}
void negozio_chiudi(void) { NEG.aperto = false; }
void negozio_esegui(int r) {
    if (r < 0 || r >= NEG.nrighe) return;
    int id = NEG.righe[r], p = NEG.prezzi[r];
    if (NEG.vendita) {
        if (!item_togli(id)) return;
        S.monete += p; if (S.nmag < 16) S.mag[S.nmag++] = id;
        toast("Venduto: %s (+%d monete)", ITEMS[id].nome, p); partita_salva();
        dlg_apri(NPC_ZAMBU, "Zambu"); dlg_pag(id == IT_ORO ? DICE_ORO : id == IT_SHOTA ? DICE_SHOTA : "Affare fatto.", false);
        dlg_sc("Vendi altro", A_VENDI); dlg_sc("Arrivederci", A_CHIUDI);
    } else {
        if (S.monete < p) { toast("Non hai abbastanza monete"); return; }
        if (!item_dai(id, 1)) { toast("Lo zaino è pieno"); return; }
        S.monete -= p;
        for (int i = 0; i < S.nmag; i++) if (S.mag[i] == id) { memmove(&S.mag[i], &S.mag[i + 1], S.nmag - i - 1); S.nmag--; break; }
        toast("Comprato: %s", ITEMS[id].nome); partita_salva(); negozio_apri(false);
    }
}

/* ---------------- personaggi vicini ---------------- */
bool npc_vicino(int *chi, float *ox, float *oy) {
    float best = 3.5f * T; int c = 0; float bx = 0, by = 0;
    #define PROVA(k, x, y) { float d = hypotf(S.px - (x), S.py - (y)); if (d < best) { best = d; c = k; bx = x; by = y; } }
    if (SI.c) PROVA(NPC_SIRO, SI.tx * T + 8.f, (SI.ty + 1.f) * T);
    if (ZB.c && ZB.stato != ZB_VIA) PROVA(NPC_ZAMBU, ZB.x, ZB.y);
    if (PS.ok && S.mappa == 0) PROVA(NPC_ANDREA, PS.x, PS.y);
    if (chi) *chi = c; if (ox) *ox = bx; if (oy) *oy = by;
    return c != 0;
}
static void parla(int chi) { if (chi == NPC_SIRO) siro_parla(); else if (chi == NPC_ZAMBU) zambu_parla(); else if (chi == NPC_ANDREA) andrea_parla(); }

/* ---------------- ciclo ---------------- */
void gioco_inizia(void) {
    IT_SHOTA = item_cerca("shota"); IT_ORO = item_cerca("shotaDorata");
    UI.luci = true; UI.vignetta = true; UI.effetti = true; UI.bagliore = true;
    atmo_inizia();
    stato_base(); S.schermata = SCH_TITOLO;
    Salva v; S.c_salvataggio = salva_leggi(&v);
    entra(0, 0, 0, true); S.minuti = 18.4f * 60;               /* sfondo del titolo: il parco al tramonto */
}
static void cielo(float dt) {
    float h = ora_del_giorno(), t[3];
    int i = 0; while (i < NSKY - 2 && SKY[i + 1][0] <= h) i++;
    float k = clampf((h - SKY[i][0]) / (SKY[i + 1][0] - SKY[i][0]), 0, 1);
    for (int c = 0; c < 3; c++) t[c] = SKY[i][c + 1] + (SKY[i + 1][c + 1] - SKY[i][c + 1]) * k;
    if (!S.amb_ok) { memcpy(S.amb, t, sizeof t); S.amb_ok = true; }
    float kk = fminf(1, dt * .8f); for (int c = 0; c < 3; c++) S.amb[c] += (t[c] - S.amb[c]) * kk;
    float lum = (S.amb[0] + S.amb[1] + S.amb[2]) / 3;
    S.notte = clampf((.86f - lum) / .46f, 0, 1); S.fuori = clampf((lum - .3f) / .6f, 0, 1);
}
static void raccogli(void) {
    Mappa *m = M_ATT(); int tx = (int)floorf(S.px / T), ty = (int)floorf((S.py - 3) / T);
    for (int i = 0; i < m->nracc && i < 8; i++) {
        Racc *o = &m->racc[i]; if (o->x != tx || o->y != ty || (S.raccolti[S.mappa] & (1 << i))) continue;
        int it = o->item; if (it < 0 || it >= R.nitem) continue;
        if (!item_dai(it, 1)) { toast("Lo zaino è pieno"); continue; }
        S.raccolti[S.mappa] |= 1 << i;
        for (int b = 0; b < 4; b++) if (BRILLI[b].t <= 0 || BRILLI[b].t > .7f) { BRILLI[b] = (Brillo){ o->x * T + 8.f, o->y * T + 8.f, .001f }; break; }
        if (it == IT_SHOTA && S.q_stato == Q_ATTIVA) { if (S.q_passo < 2) S.q_passo = 2; toast("Hai trovato la Shota di Siro! Riportagliela"); }
        else toast("Hai raccolto: %s", ITEMS[it].nome);
        partita_salva();
    }
}
void gioco_passo(float dt, const Input *in) {
    if (dt > .1f) dt = .1f;
    S.t += dt;
    for (int i = 0; i < 3; i++) if (TOAST[i].vita > 0) TOAST[i].vita -= dt;
    for (int b = 0; b < 4; b++) if (BRILLI[b].t > 0) BRILLI[b].t += dt;
    if (S.nome_luogo > 0) S.nome_luogo -= dt;
    if (S.festa < 99) S.festa += dt;
    if (S.schermata == SCH_TITOLO) { S.minuti += dt * 4; if (S.minuti > 21 * 60) S.minuti = 17 * 60; cielo(dt); ui_titolo_input(in); ps_aggiorna(dt); atmo_passo(dt, S.camx, S.camy, M_ATT()->interno, S.notte, ora_del_giorno()); return; }

    bool usato = ui_input(in);
    if (DLG.aperto) { DLG.t += dt; int n = (int)(DLG.t / .03f); if (n > DLG.n) DLG.n = n; }
    bool fermo = usato || DLG.aperto || NEG.aperto || S.fade >= 0 || S.festa < FESTA_DUR;

    /* tempo: 1 minuto di gioco ogni 0,7 s; alle 2:00 si va a dormire e riparte alle 6:00 del giorno dopo */
    S.minuti += dt * 10 / 7.f;
    if (S.minuti >= 26 * 60) { S.minuti = 6 * 60; S.giorno++; S.amb_ok = false; toast("Si è fatto tardi... Giorno %d", S.giorno); partita_salva(); }
    cielo(dt);                                       /* al chiuso la luce la decide mondo.c */

    /* movimento */
    float ax = fermo ? 0 : in->ax, ay = fermo ? 0 : in->ay, l = hypotf(ax, ay);
    S.moving = l > .15f;
    if (S.moving) {
        if (l > 1) { ax /= l; ay /= l; }
        float v = 75 * ((in->tenuti & TS_B) ? .45f : 1) * dt, mx = ax * v, my = ay * v;
        S.dir = fabsf(ax) > fabsf(ay) ? (ax > 0 ? D_DX : D_SX) : (ay > 0 ? D_GIU : D_SU);
        float ox = S.px, oy = S.py;
        if (!bloccato(S.px + mx, S.py)) S.px += mx;
        if (!bloccato(S.px, S.py + my)) S.py += my;
        float d = hypotf(S.px - ox, S.py - oy); S.dist += d;
        if (d < .01f) S.moving = false;
    }
    if (!fermo && (in->giu & TS_A)) { int chi; if (npc_vicino(&chi, NULL, NULL)) parla(chi); }

    /* passaggi con dissolvenza */
    if (S.fade >= 0) {
        S.fade += dt;
        if (!S.fade_fatto && S.fade >= .22f) { S.fade_fatto = true; entra(S.fade_to, S.fade_ax, S.fade_ay, false); }
        if (S.fade >= .55f) S.fade = -1;
    } else {
        int pi = passo_in((int)floorf(S.px / T), (int)floorf((S.py - 3) / T));
        if (pi != S.passo_dentro) {
            S.passo_dentro = pi;
            if (pi >= 0) { Passo *p = &M_ATT()->passi[pi]; if (p->to < NMAPPE) { S.fade = 0; S.fade_fatto = false; S.fade_to = p->to; S.fade_ax = p->ax; S.fade_ay = p->ay; } }
        }
    }
    raccogli();
    zb_aggiorna(dt);
    ps_aggiorna(dt);
    atmo_passo(dt, S.camx, S.camy, M_ATT()->interno, S.notte, ora_del_giorno());
}
void debug_entra(int m, int x, int y) { entra(m, x, y, false); }
