/* interfaccia: sopra orologio, avvisi e dialogo; sotto zaino, quest, messaggi, mappa, opzioni (touch e tasti) */
#include "stato.h"
#include "dati.h"

/* colori: pergamena e legno, come i pannelli del gioco */
#define C_CARTA   rgb(246, 233, 205)
#define C_CARTA2  rgb(232, 214, 178)
#define C_BORDO   rgb(92, 58, 33)
#define C_LEGNO   rgb(122, 82, 48)
#define C_LEGNO2  rgb(84, 55, 32)
#define C_INCH    rgb(58, 38, 24)
#define C_TENUE   rgb(128, 98, 70)
#define C_ORO     rgb(240, 192, 64)
#define C_VERDE   rgb(92, 150, 70)
#define C_FONDO   rgb(36, 28, 24)

static void pannello(Sup *s, int x, int y, int w, int h, uint16_t fondo) {
    sup_rett(s, x + 1, y, w - 2, h, C_BORDO); sup_rett(s, x, y + 1, w, h - 2, C_BORDO);
    sup_rett(s, x + 2, y + 2, w - 4, h - 4, fondo);
    sup_rett(s, x + 2, y + h - 3, w - 4, 1, mix565(fondo, C_BORDO, 70));
}
static void bottone(Sup *s, int x, int y, int w, int h, const char *t, bool sel, bool attivo) {
    uint16_t f = !attivo ? C_CARTA2 : sel ? C_ORO : C_CARTA;
    pannello(s, x, y, w, h, f);
    int tw = testo_largo(t); testo(s, x + (w - tw) / 2, y + (h - 10) / 2 + 1, t, attivo ? C_INCH : C_TENUE);
}
static bool dentro(const Input *in, int x, int y, int w, int h) { return in->tocco_nuovo && in->tx >= x && in->ty >= y && in->tx < x + w && in->ty < y + h; }
static void moneta(Sup *s, int x, int y, int n) { char b[24]; snprintf(b, sizeof b, "%d", n); sup_img(s, img("moneta"), x, y, 256); testo(s, x + 12, y, b, C_INCH); }

/* ---------- ritratto: la figura ingrandita su un fondo ---------- */
void ui_ritratto(Sup *s, int chi, int x, int y, int z) {
    int W = 16 * z + 8, H = 18 * z + 8; bool notte = chi == NPC_ZAMBU;
    for (int j = 0; j < H; j++) sup_rett(s, x, y + j, W, 1, mix565(notte ? rgb(29, 36, 51) : rgb(143, 180, 201), notte ? rgb(13, 17, 24) : rgb(93, 125, 140), j * 256 / H));
    if (notte) sup_rett_a(s, x + W / 2 - 4, y + H * 2 / 3, 14, 10, rgb(255, 140, 60), 46);
    const Img *im = chi == NPC_SIRO ? img("siro_down") : chi == NPC_ZAMBU ? img("zambu_giu") : img("andrea");
    sup_img_scala(s, im, 0, 3, 16, 18, x + 4, y + 6, z);
    sup_rett(s, x, y, W, 1, C_BORDO); sup_rett(s, x, y + H - 1, W, 1, C_BORDO); sup_rett(s, x, y, 1, H, C_BORDO); sup_rett(s, x + W - 1, y, 1, H, C_BORDO);
}

static int lung(const char *t) { int n = 0; for (; *t; t++) if ((*t & 0xC0) != 0x80) n++; return n; }
static bool dlg_fine(void) { return DLG.n >= lung(DLG.pag[DLG.i]); }

/* ================= schermo sopra ================= */
void ui_disegna_sopra(Sup *s) {
    if (S.schermata == SCH_TITOLO) {
        sup_rett_a(s, 0, 52, SW, 74, 0, 110);
        const char *t1 = "La Valle", *t2 = "dei Germogli";
        testo_grande(s, (SW - testo_largo(t1) * 3) / 2, 58, t1, rgb(255, 241, 191), rgb(42, 20, 8), 3);
        testo_grande(s, (SW - testo_largo(t2) * 3) / 2, 90, t2, rgb(255, 241, 191), rgb(42, 20, 8), 3);
        const char *t3 = "Edizione New Nintendo 3DS";
        testo_ombra(s, (SW - testo_largo(t3)) / 2, 224, t3, rgb(255, 241, 191), 0);
        return;
    }
    /* orologio, giorno e monete */
    char b[64]; int h = (int)(S.minuti / 60) % 24, mi = ((int)S.minuti % 60) / 10 * 10;
    pannello(s, 4, 4, 92, 36, C_CARTA);
    snprintf(b, sizeof b, "Giorno %d", S.giorno); testo(s, 10, 8, b, C_INCH);
    snprintf(b, sizeof b, "%d:%02d", h, mi); testo(s, 96 - 8 - testo_largo(b), 8, b, C_INCH);
    moneta(s, 10, 24, S.monete);
    float ora = ora_del_giorno(); sup_rett(s, 72, 25, 16, 9, mix565(rgb(120, 170, 220), rgb(30, 40, 80), (int)(S.notte * 256)));
    if (ora >= 6 && ora < 19.5f) sup_rett(s, 72 + (int)((ora - 6) / 13.5f * 13), 27, 3, 3, rgb(255, 214, 90));
    else { float k = fmodf(ora + 24 - 19.5f, 24) / 10.5f; sup_rett(s, 72 + (int)(clampf(k, 0, 1) * 13), 27, 3, 3, rgb(230, 235, 255)); }
    /* nome del luogo appena entrati */
    if (S.nome_luogo > 0) {
        const char *n = M_ATT()->nome; int w = testo_largo(n) + 16, a = (int)(clampf(S.nome_luogo, 0, 1) * 256);
        sup_rett_a(s, (SW - w) / 2, 8, w, 18, C_FONDO, a * 3 / 4); if (a > 128) testo(s, (SW - w) / 2 + 8, 12, n, rgb(255, 241, 191));
    }
    /* avvisi */
    int ty = DLG.aperto ? 128 : SH - 22;                      /* in basso, per non coprire il personaggio */
    for (int i = 2; i >= 0; i--) {
        if (TOAST[i].vita <= 0) continue;
        int w = testo_largo(TOAST[i].t) + 14, a = (int)(clampf(TOAST[i].vita * 2, 0, 1) * 256);
        if (w > SW - 8) w = SW - 8;
        sup_rett_a(s, (SW - w) / 2, ty, w, 16, C_FONDO, a * 4 / 5);
        if (a > 100) testo(s, (SW - w) / 2 + 7, ty + 3, TOAST[i].t, rgb(255, 241, 191));
        ty -= 18;
    }
    /* «A» sopra il personaggio con cui si può parlare */
    int chi; float nx, ny;
    if (!DLG.aperto && !NEG.aperto && S.fade < 0 && npc_vicino(&chi, &nx, &ny)) {
        int x = (int)nx - S.camx, y = (int)ny - S.camy - 44 + ((int)(S.t * 3) % 2);
        pannello(s, x - 6, y, 13, 13, C_ORO); testo(s, x - 2, y + 2, "A", C_INCH);
    }
    /* dialogo */
    if (DLG.aperto) {
        int y0 = 160;
        pannello(s, 6, y0, SW - 12, SH - y0 - 4, C_CARTA);
        ui_ritratto(s, DLG.chi, 12, y0 + 7, 3);
        int nw = testo_largo(DLG.nome) + 12; pannello(s, 12, y0 - 13, nw, 16, C_LEGNO); testo(s, 18, y0 - 10, DLG.nome, C_CARTA);
        testo_a_capo(s, 76, y0 + 8, SW - 92, DLG.pag[DLG.i], C_INCH, DLG.n);
        bool fine = dlg_fine();
        if (fine && DLG.i < DLG.npag - 1 && ((int)(S.t * 3) % 2)) testo(s, SW - 22, SH - 18, "▶", C_LEGNO);
        if (fine && DLG.i == DLG.npag - 1) testo(s, SW - 16 - testo_largo("Scegli in basso ↓"), SH - 18, "Scegli in basso ↓", C_TENUE);
    }
}

/* ================= schermo sotto ================= */
static const char *TAB_ICONE[NTAB] = { "icona_zaino", "icona_quest", "icona_messaggi", "icona_mappa", "icona_impostazioni" };
static void slot_xy(int i, int *x, int *y) { *x = 16 + (i % 9) * 32; *y = 52 + (i / 9) * 32 + (i >= 9 ? 4 : 0); }
static bool dlg_scelte(void) { return DLG.i == DLG.npag - 1 && dlg_fine(); }

static void icona_item(Sup *s, int item, int x, int y) {
    const ItemDef *d = &ITEMS[item]; const Img *im = d->frames ? d->fr[(int)(S.t * 9) % d->frames] : d->icona; sup_img(s, im, x, y, 256);
}
static void sotto_zaino(Sup *s) {
    testo(s, 12, 36, "Zaino", C_INCH); moneta(s, BW - 16 - 12 - testo_largo("0000"), 36, S.monete);
    for (int i = 0; i < NSLOT; i++) {
        int x, y; slot_xy(i, &x, &y);
        uint16_t f = i < NBARRA ? rgb(226, 204, 160) : C_CARTA2;
        sup_rett(s, x, y, 30, 30, C_BORDO); sup_rett(s, x + 1, y + 1, 28, 28, f);
        if (i == UI.sel) { sup_rett(s, x - 1, y - 1, 32, 2, C_LEGNO); sup_rett(s, x - 1, y + 29, 32, 2, C_LEGNO); sup_rett(s, x - 1, y - 1, 2, 32, C_LEGNO); sup_rett(s, x + 29, y - 1, 2, 32, C_LEGNO); }
        if (i == S.mano) { sup_rett(s, x, y, 30, 2, C_ORO); sup_rett(s, x, y + 28, 30, 2, C_ORO); sup_rett(s, x, y, 2, 30, C_ORO); sup_rett(s, x + 28, y, 2, 30, C_ORO); }
        if (S.inv[i].item >= 0) {
            icona_item(s, S.inv[i].item, x + 7, y + 7);
            if (S.inv[i].n > 1) { char b[8]; snprintf(b, sizeof b, "%d", S.inv[i].n); testo_ombra(s, x + 28 - testo_largo(b), y + 19, b, 0xFFFF, C_INCH); }
        }
    }
    int y = 190; sup_rett(s, 12, y - 3, BW - 24, 1, C_CARTA2);
    if (UI.sel >= 0 && S.inv[UI.sel].item >= 0) {
        const ItemDef *d = &ITEMS[S.inv[UI.sel].item];
        testo(s, 14, y, d->nome, C_LEGNO2); testo_a_capo(s, 14, y + 13, BW - 28, d->lore, C_TENUE, -1);
    } else if (UI.sel >= 0) testo(s, 14, y, "Tocca un altro posto per spostare. Tocca ancora per lasciare.", C_TENUE);
    else testo_a_capo(s, 14, y, BW - 28, "La prima fila è la barra: L e R scelgono l'oggetto in mano. Tocca un oggetto per leggerlo, poi un altro posto per spostarlo.", C_TENUE, -1);
}
static void casella(Sup *s, int x, int y, bool piena) { sup_rett(s, x, y, 8, 8, C_BORDO); sup_rett(s, x + 1, y + 1, 6, 6, piena ? C_VERDE : C_CARTA); }
static void sotto_quest(Sup *s) {
    testo(s, 12, 36, "Quest", C_INCH);
    if (S.q_stato == Q_NO) { testo_a_capo(s, 14, 60, BW - 28, "Nessuna quest per ora. Parla con gli abitanti della Valle: qualcuno avrà bisogno di te.", C_TENUE, -1); return; }
    int y = 54; testo(s, 14, y, Q_TITOLO, C_LEGNO2);
    if (S.q_stato == Q_FATTA) testo(s, BW - 14 - testo_largo("Completata"), y, "Completata", C_VERDE);
    y += 13; char b[128]; snprintf(b, sizeof b, "Da: %s · %s", Q_DA, Q_DOVE); testo(s, 14, y, b, C_TENUE); y += 15;
    for (int i = 0; i < NQDESCR; i++) y += testo_a_capo(s, 14, y, BW - 28, Q_DESCR[i], C_INCH, -1) + 2;
    y += 4;
    for (int i = 0; i < NQPASSI; i++) {
        bool fatto = i < S.q_passo || S.q_stato == Q_FATTA; casella(s, 14, y + 1, fatto);
        testo(s, 28, y, Q_PASSI[i], fatto ? C_TENUE : C_INCH); if (fatto) sup_rett(s, 28, y + 5, testo_largo(Q_PASSI[i]), 1, C_TENUE); y += 13;
    }
    snprintf(b, sizeof b, "Premio: %s", S.q_stato == Q_FATTA ? "Shota Dorata" : Q_PREMIO); testo(s, 14, y + 4, b, C_LEGNO);
}
static void sotto_msg(Sup *s) {
    testo(s, 12, 36, "Messaggi", C_INCH);
    pannello(s, 12, 52, BW - 24, 180, rgb(252, 246, 230));
    testo(s, 22, 60, MSG_TITOLO, C_LEGNO2); char b[64]; snprintf(b, sizeof b, "Da: %s · Giorno 1", MSG_DA); testo(s, 22, 73, b, C_TENUE);
    sup_rett(s, 22, 86, BW - 44, 1, C_CARTA2);
    testo_a_capo(s, 22, 92, BW - 44, MSG_TESTO, C_INCH, -1);
}
static void sotto_mappa(Sup *s) {
    Mappa *m = M_ATT(); testo(s, 12, 36, m->nome, C_INCH);
    int x0 = (BW - m->mw) / 2, y0 = 52 + (180 - m->mh) / 2;
    sup_rett(s, x0 - 2, y0 - 2, m->mw + 4, m->mh + 4, C_BORDO);
    for (int j = 0; j < m->mh; j++) memcpy(s->px + (y0 + j) * BW + x0, m->mini + j * m->mw, m->mw * 2);
    float kx = (float)m->mw / (m->W * T), ky = (float)m->mh / (m->H * T);
    if (SI.c) sup_rett(s, x0 + (int)((SI.tx * T + 8) * kx) - 1, y0 + (int)((SI.ty * T + 8) * ky) - 1, 3, 3, rgb(122, 90, 168));
    if (ZB.c && ZB.stato != ZB_VIA) sup_rett(s, x0 + (int)(ZB.x * kx) - 1, y0 + (int)(ZB.y * ky) - 2, 3, 3, rgb(40, 40, 46));
    if (PS.ok && S.mappa == 0) sup_rett(s, x0 + (int)(PS.x * kx) - 1, y0 + (int)(PS.y * ky) - 2, 3, 3, rgb(217, 138, 160));
    if ((int)(S.t * 3) % 3) { int px = x0 + (int)(S.px * kx), py = y0 + (int)(S.py * ky); sup_rett(s, px - 2, py - 2, 5, 5, 0xFFFF); sup_rett(s, px - 1, py - 1, 3, 3, rgb(220, 40, 40)); }
}
static const char *SN(bool v) { return v ? "Sì" : "No"; }
static void sotto_opz(Sup *s) {
    testo(s, 12, 36, "Opzioni", C_INCH); char b[48];
    bottone(s, 16, 52, 140, 26, "Salva ora", false, true);
    bottone(s, 164, 52, 140, 26, "Torna al titolo", false, true);
    snprintf(b, sizeof b, "Luci notturne: %s", SN(UI.luci)); bottone(s, 16, 84, 140, 26, b, false, true);
    snprintf(b, sizeof b, "Vignettatura: %s", SN(UI.vignetta)); bottone(s, 164, 84, 140, 26, b, false, true);
    bottone(s, 16, 116, 288, 26, "Salva ed esci", false, true);
    int y = 150; testo(s, 14, y, "Comandi", C_LEGNO2);
    snprintf(b, sizeof b, "%d fps", S.fps); testo(s, BW - 14 - testo_largo(b), y, b, C_TENUE); y += 13;
    static const char *C[] = { "Levetta / Croce: cammina", "Tieni premuto B: cammina piano", "A: parla, conferma", "L / R: oggetto in mano",
        "X: scheda successiva   Y: scheda precedente", "START: opzioni   SELECT: mappa" };
    for (int i = 0; i < 6; i++) { testo(s, 18, y, C[i], C_INCH); y += 12; }
}
static void sotto_dialogo(Sup *s) {
    ui_ritratto(s, DLG.chi, 12, 12, 4);
    testo_grande(s, 92, 16, DLG.nome, C_CARTA, C_FONDO, 2);
    if (!dlg_scelte()) {
        const char *t = DLG.i < DLG.npag - 1 || !dlg_fine() ? "Tocca o premi A per continuare" : "";
        testo(s, 92, 50, t, C_CARTA2);
        return;
    }
    for (int i = 0; i < DLG.nsc; i++) bottone(s, 16, 104 + i * 32, BW - 32, 28, DLG.sc[i].t, i == DLG.sel, true);
}
static void sotto_negozio(Sup *s) {
    pannello(s, 6, 6, BW - 12, BH - 12, C_CARTA);
    testo(s, 16, 14, NEG.vendita ? "Zambu compra" : "Zambu vende", C_LEGNO2); moneta(s, BW - 16 - 12 - testo_largo("0000"), 14, S.monete);
    testo_a_capo(s, 16, 30, BW - 32, NEG.testa, C_TENUE, -1);
    for (int r = 0; r < NEG.nrighe; r++) {
        int y = 58 + r * 30, id = NEG.righe[r]; bool sel = r == NEG.sel;
        sup_rett(s, 14, y, BW - 28, 28, sel ? rgb(250, 226, 160) : C_CARTA2);
        icona_item(s, id, 20, y + 6); testo(s, 42, y + 9, ITEMS[id].nome, C_INCH);
        char b[24]; snprintf(b, sizeof b, "%s%d", NEG.vendita ? "+" : "", NEG.prezzi[r]); sup_img(s, img("moneta"), 168, y + 9, 256); testo(s, 180, y + 9, b, C_INCH);
        bottone(s, BW - 84, y + 3, 64, 22, NEG.vendita ? "Vendi" : "Compra", sel, NEG.vendita || S.monete >= NEG.prezzi[r]);
    }
    bottone(s, (BW - 100) / 2, BH - 40, 100, 26, "Chiudi", false, true);
}
void ui_disegna_sotto(Sup *s) {
    sup_riempi(s, C_FONDO);
    if (S.schermata == SCH_TITOLO) {
        pannello(s, 40, 40, BW - 80, 160, C_CARTA);
        testo(s, (BW - testo_largo("Benvenuto!")) / 2, 54, "Benvenuto!", C_LEGNO2);
        int y = 80;
        if (UI.conferma_nuova) {
            testo_a_capo(s, 56, 74, BW - 112, "Iniziare da capo? Il salvataggio attuale verrà sostituito.", C_INCH, -1);
            bottone(s, 56, 120, 96, 30, "Sì", UI.titolo_sel == 0, true); bottone(s, BW - 152, 120, 96, 30, "No", UI.titolo_sel == 1, true);
            return;
        }
        if (S.c_salvataggio) { bottone(s, 64, y, BW - 128, 32, "Continua", UI.titolo_sel == 0, true); y += 42; }
        bottone(s, 64, y, BW - 128, 32, "Nuova partita", UI.titolo_sel == (S.c_salvataggio ? 1 : 0), true);
        testo(s, (BW - testo_largo("Croce + A oppure tocca")) / 2, 178, "Croce + A oppure tocca", C_TENUE);
        return;
    }
    if (NEG.aperto) { sotto_negozio(s); return; }
    if (DLG.aperto) { sotto_dialogo(s); return; }
    /* schede */
    for (int i = 0; i < NTAB; i++) {
        int x = i * 64; bool on = i == UI.tab;
        sup_rett(s, x, 0, 64, 30, on ? C_CARTA : C_LEGNO); sup_rett(s, x + 63, 0, 1, 30, C_LEGNO2);
        const Img *ic = img(TAB_ICONE[i]); if (ic) sup_img_scala(s, ic, 0, 0, ic->w, ic->h, x + 20, 3, 2);
        if (i == TAB_MSG && !S.msg_letto) { sup_rett(s, x + 46, 3, 9, 11, C_BORDO); sup_rett(s, x + 47, 4, 7, 9, rgb(230, 70, 60)); testo(s, x + 49, 4, "!", 0xFFFF); }
        if (i == TAB_QUEST && S.q_stato == Q_ATTIVA && S.q_passo == 2) { sup_rett(s, x + 46, 3, 9, 11, C_BORDO); sup_rett(s, x + 47, 4, 7, 9, C_ORO); testo(s, x + 49, 4, "!", C_INCH); }
    }
    sup_rett(s, 0, 30, BW, BH - 30, C_CARTA); sup_rett(s, 0, 30, BW, 1, C_BORDO);
    switch (UI.tab) {
    case TAB_ZAINO: sotto_zaino(s); break;
    case TAB_QUEST: sotto_quest(s); break;
    case TAB_MSG: sotto_msg(s); S.msg_letto = true; break;
    case TAB_MAPPA: sotto_mappa(s); break;
    case TAB_OPZ: sotto_opz(s); break;
    }
}

/* ================= input ================= */
void ui_titolo_input(const Input *in) {
    if (UI.conferma_nuova) {
        if (in->giu & (TS_SX | TS_DX)) UI.titolo_sel ^= 1;
        bool si = dentro(in, 56, 120, 96, 30) || ((in->giu & TS_A) && UI.titolo_sel == 0);
        bool no = dentro(in, BW - 152, 120, 96, 30) || ((in->giu & TS_A) && UI.titolo_sel == 1) || (in->giu & TS_B);
        if (si) { UI.conferma_nuova = false; partita_nuova(); } else if (no) { UI.conferma_nuova = false; UI.titolo_sel = 0; }
        return;
    }
    int n = S.c_salvataggio ? 2 : 1;
    if (in->giu & TS_SU) UI.titolo_sel = (UI.titolo_sel + n - 1) % n;
    if (in->giu & TS_GIU) UI.titolo_sel = (UI.titolo_sel + 1) % n;
    int scelta = -1;
    if (in->giu & TS_A) scelta = UI.titolo_sel;
    if (S.c_salvataggio && dentro(in, 64, 80, BW - 128, 32)) scelta = 0;
    if (dentro(in, 64, S.c_salvataggio ? 122 : 80, BW - 128, 32)) scelta = n - 1;
    if (scelta < 0) return;
    if (S.c_salvataggio && scelta == 0) { if (!partita_carica()) { toast("Salvataggio non leggibile"); S.c_salvataggio = false; } }
    else if (S.c_salvataggio) { UI.conferma_nuova = true; UI.titolo_sel = 1; }
    else partita_nuova();
}
bool ui_input(const Input *in) {
    if (NEG.aperto) {
        if (in->giu & TS_SU) NEG.sel = clampi(NEG.sel - 1, 0, NEG.nrighe ? NEG.nrighe - 1 : 0);
        if (in->giu & TS_GIU) NEG.sel = clampi(NEG.sel + 1, 0, NEG.nrighe ? NEG.nrighe - 1 : 0);
        if (in->giu & TS_B || dentro(in, (BW - 100) / 2, BH - 40, 100, 26)) { negozio_chiudi(); return true; }
        if (in->giu & TS_A) { negozio_esegui(NEG.sel); return true; }
        for (int r = 0; r < NEG.nrighe; r++) {
            int y = 58 + r * 30;
            if (dentro(in, BW - 84, y + 3, 64, 22)) { NEG.sel = r; negozio_esegui(r); return true; }
            if (dentro(in, 14, y, BW - 28, 28)) NEG.sel = r;
        }
        return true;
    }
    if (DLG.aperto) {
        if (dlg_scelte()) {
            if (in->giu & TS_SU) DLG.sel = (DLG.sel + DLG.nsc - 1) % DLG.nsc;
            if (in->giu & TS_GIU) DLG.sel = (DLG.sel + 1) % DLG.nsc;
            if (in->giu & TS_A) { azione(DLG.sc[DLG.sel].a); return true; }
            for (int i = 0; i < DLG.nsc; i++) if (dentro(in, 16, 104 + i * 32, BW - 32, 28)) { azione(DLG.sc[i].a); return true; }
        } else if ((in->giu & (TS_A | TS_B)) || in->tocco_nuovo) dialogo_avanza();
        return true;
    }
    /* tasti rapidi */
    if (in->giu & TS_L) S.mano = (S.mano + NBARRA - 1) % NBARRA;
    if (in->giu & TS_R) S.mano = (S.mano + 1) % NBARRA;
    if (in->giu & TS_X) UI.tab = (UI.tab + 1) % NTAB;
    if (in->giu & TS_Y) UI.tab = (UI.tab + NTAB - 1) % NTAB;
    if (in->giu & TS_START) UI.tab = TAB_OPZ;
    if (in->giu & TS_SELECT) UI.tab = TAB_MAPPA;
    if (!in->tocco_nuovo) return false;
    if (in->ty < 30) { UI.tab = clampi(in->tx / 64, 0, NTAB - 1); UI.sel = -1; return false; }
    if (UI.tab == TAB_ZAINO) {
        for (int i = 0; i < NSLOT; i++) {
            int x, y; slot_xy(i, &x, &y); if (!dentro(in, x, y, 30, 30)) continue;
            if (UI.sel == i) UI.sel = -1;
            else if (UI.sel >= 0 && S.inv[UI.sel].item >= 0) { Slot t = S.inv[i]; S.inv[i] = S.inv[UI.sel]; S.inv[UI.sel] = t; UI.sel = -1; }
            else UI.sel = S.inv[i].item >= 0 ? i : -1;
            if (i < NBARRA) S.mano = i;
            return false;
        }
    }
    if (UI.tab == TAB_OPZ) {
        if (dentro(in, 16, 52, 140, 26)) { partita_salva(); toast("Partita salvata"); }
        if (dentro(in, 164, 52, 140, 26)) { partita_salva(); S.schermata = SCH_TITOLO; S.c_salvataggio = true; UI.titolo_sel = 0; UI.conferma_nuova = false; }
        if (dentro(in, 16, 84, 140, 26)) UI.luci = !UI.luci;
        if (dentro(in, 164, 84, 140, 26)) UI.vignetta = !UI.vignetta;
        if (dentro(in, 16, 116, 288, 26)) gioco_chiedi_uscita();
    }
    return false;
}

/* ================= disegno complessivo ================= */
void gioco_disegna(Sup *sopra, Sup *sotto) {
    mondo_disegna(sopra, S.schermata == SCH_TITOLO);
    ui_disegna_sopra(sopra);
    ui_disegna_sotto(sotto);
}
