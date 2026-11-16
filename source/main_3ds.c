/* livello di piattaforma per New Nintendo 3DS (libctru): schermi RGB565, tasti, touch, salvataggio sulla SD */
#include <3ds.h>
#include <sys/stat.h>
#include "stato.h"

static uint16_t SOPRA[SW * SH], SOTTO[BW * BH];

/* il framebuffer del 3DS è ruotato: colonne da 240 pixel, dal basso verso l'alto */
static void copia(gfxScreen_t scr, const uint16_t *src, int w) {
    uint16_t *fb = (uint16_t *)gfxGetFramebuffer(scr, GFX_LEFT, NULL, NULL);
    for (int x = 0; x < w; x++) {
        uint16_t *d = fb + x * 240 + 239; const uint16_t *s = src + x;
        for (int y = 0; y < 240; y++, s += w) *d-- = *s;
    }
}
static void schermata_testo(const char *riga1, const char *riga2) {
    Sup a = { SOPRA, SW, SH };
    sup_riempi(&a, rgb(36, 28, 24));
    testo(&a, (SW - testo_largo(riga1)) / 2, 110, riga1, rgb(255, 241, 191));
    if (riga2) testo(&a, (SW - testo_largo(riga2)) / 2, 126, riga2, rgb(200, 180, 150));
    copia(GFX_TOP, SOPRA, SW);
    memset(SOTTO, 0, sizeof SOTTO); copia(GFX_BOTTOM, SOTTO, BW);
    gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();
}
/* il gioco si salva anche quando si chiude il coperchio o si torna al menu HOME */
static aptHookCookie GANCIO;
static void su_apt(APT_HookType t, void *p) { (void)p; if (t == APTHOOK_ONSUSPEND || t == APTHOOK_ONSLEEP || t == APTHOOK_ONEXIT) gioco_salva_ora(); }

int main(void) {
    osSetSpeedupEnable(true);                      /* New 3DS: 804 MHz e cache L2 */
    gfxInitDefault();
    gfxSetScreenFormat(GFX_TOP, GSP_RGB565_OES);
    gfxSetScreenFormat(GFX_BOTTOM, GSP_RGB565_OES);
    gfxSetDoubleBuffering(GFX_TOP, true); gfxSetDoubleBuffering(GFX_BOTTOM, true);
    gfxSet3D(false);
    schermata_testo("Caricamento...", NULL);
    Result rr = romfsInit();
    mkdir("sdmc:/3ds", 0777); mkdir("sdmc:/3ds/ValleDeiGermogli", 0777);
    snprintf(PERCORSO_SALVA, sizeof PERCORSO_SALVA, "sdmc:/3ds/ValleDeiGermogli/salvataggio.bin");
    if (R_FAILED(rr) || !risorse_carica("romfs:/")) {
        schermata_testo("Non riesco a leggere i dati del gioco.", "Reinstalla il file .cia. Premi START per uscire.");
        while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_START) break; gspWaitForVBlank(); }
        romfsExit(); gfxExit(); return 0;
    }
    aptHook(&GANCIO, su_apt, NULL);
    gioco_inizia();
    Sup sopra = { SOPRA, SW, SH }, sotto = { SOTTO, BW, BH };
    u64 prima = osGetTime(), sec = prima; int fotogrammi = 0;
    while (aptMainLoop() && !gioco_esci()) {
        hidScanInput();
        u32 giu = hidKeysDown(), tenuti = hidKeysHeld();
        Input in; memset(&in, 0, sizeof in);
        static const u32 K[12] = { KEY_A, KEY_B, KEY_X, KEY_Y, KEY_L | KEY_ZL, KEY_R | KEY_ZR, KEY_START, KEY_SELECT, KEY_DUP, KEY_DDOWN, KEY_DLEFT, KEY_DRIGHT };
        for (int i = 0; i < 12; i++) { if (giu & K[i]) in.giu |= 1u << i; if (tenuti & K[i]) in.tenuti |= 1u << i; }
        circlePosition cp; hidCircleRead(&cp);
        float ax = cp.dx / 150.f, ay = -cp.dy / 150.f;
        if (fabsf(ax) < .18f) ax = 0;
        if (fabsf(ay) < .18f) ay = 0;
        if (tenuti & KEY_DLEFT) ax = -1;
        if (tenuti & KEY_DRIGHT) ax = 1;
        if (tenuti & KEY_DUP) ay = -1;
        if (tenuti & KEY_DDOWN) ay = 1;
        in.ax = clampf(ax, -1, 1); in.ay = clampf(ay, -1, 1);
        if (tenuti & KEY_TOUCH) { touchPosition tp; hidTouchRead(&tp); in.tocco = true; in.tocco_nuovo = (giu & KEY_TOUCH) != 0; in.tx = tp.px; in.ty = tp.py; }
        u64 ora = osGetTime(); float dt = (ora - prima) / 1000.f; prima = ora;
        gioco_passo(dt, &in);
        gioco_disegna(&sopra, &sotto);
        copia(GFX_TOP, SOPRA, SW);
        copia(GFX_BOTTOM, SOTTO, BW);
        gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();
        fotogrammi++; if (ora - sec >= 1000) { S.fps = fotogrammi; fotogrammi = 0; sec = ora; }
    }
    gioco_salva_ora();
    aptUnhook(&GANCIO);
    romfsExit(); gfxExit();
    return 0;
}
