/* banco di prova sul PC: esegue un copione di comandi e salva le schermate dei due schermi (PPM).
   uso: main_pc <cartella romfs/> <copione> <cartella uscita/> */
#include "../source/stato.h"
#include <time.h>

static uint16_t SOPRA[SW * SH], SOTTO[BW * BH];
static void scatta(const char *dir, const char *nome) {
    char p[512]; snprintf(p, sizeof p, "%s/%s.ppm", dir, nome);
    FILE *f = fopen(p, "wb"); fprintf(f, "P6\n%d %d\n255\n", SW, SH * 2);
    for (int y = 0; y < SH * 2; y++) for (int x = 0; x < SW; x++) {
        uint16_t c = 0;
        if (y < SH) c = SOPRA[y * SW + x];
        else if (x >= 40 && x < 360) c = SOTTO[(y - SH) * BW + x - 40];
        unsigned char b[3] = { (c >> 11) << 3 | (c >> 13), ((c >> 5) & 63) << 2 | ((c >> 9) & 3), (c & 31) << 3 | ((c >> 2) & 7) };
        if (y >= SH && (x < 40 || x >= 360)) b[0] = b[1] = b[2] = 20;
        fwrite(b, 1, 3, f);
    }
    fclose(f);
}
static double ora(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static uint32_t tasto(const char *n) {
    static const char *N[] = { "A", "B", "X", "Y", "L", "R", "START", "SELECT", "SU", "GIU", "SX", "DX" };
    for (int i = 0; i < 12; i++) if (!strcmp(n, N[i])) return 1u << i;
    return 0;
}
int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "uso: %s romfs/ copione uscita/\n", argv[0]); return 1; }
    snprintf(PERCORSO_SALVA, sizeof PERCORSO_SALVA, "%s/salvataggio.bin", argv[3]);
    double t0 = ora();
    if (!risorse_carica(argv[1])) { fprintf(stderr, "risorse mancanti\n"); return 1; }
    printf("risorse caricate in %.2f s: %d tile, %d sprite, %d immagini, %d oggetti, %d mappe\n", ora() - t0, R.ntiles, R.nspr, R.nimg, R.nitem, NMAPPE);
    gioco_inizia();
    Sup sopra = { SOPRA, SW, SH }, sotto = { SOTTO, BW, BH };
    FILE *c = fopen(argv[2], "r"); char riga[256]; double tdis = 0; int ndis = 0;
    uint32_t tenuti = 0; float ax = 0, ay = 0;
    while (fgets(riga, sizeof riga, c)) {
        char cmd[32] = "", a1[64] = "", a2[64] = "", a3[64] = ""; if (sscanf(riga, "%31s %63s %63s %63s", cmd, a1, a2, a3) < 1 || cmd[0] == '#') continue;
        Input in; memset(&in, 0, sizeof in);
        int passi = 1;
        if (!strcmp(cmd, "premi")) { in.giu = tasto(a1); }
        else if (!strcmp(cmd, "tocca")) { in.tocco = in.tocco_nuovo = true; in.tx = atoi(a1); in.ty = atoi(a2); }
        else if (!strcmp(cmd, "aspetta")) { passi = (int)(atof(a1) * 60); }
        else if (!strcmp(cmd, "cammina")) { ax = atof(a1); ay = atof(a2); passi = (int)(atof(a3) * 60); }
        else if (!strcmp(cmd, "tieni")) { tenuti = tasto(a1); continue; }
        else if (!strcmp(cmd, "ora")) { S.minuti = atof(a1) * 60; S.amb_ok = false; continue; }
        else if (!strcmp(cmd, "vai")) { S.px = atoi(a1) * T + 8; S.py = atoi(a2) * T + 14; continue; }
        else if (!strcmp(cmd, "mappa")) { debug_entra(atoi(a1), atoi(a2), atoi(a3)); continue; }
        else if (!strcmp(cmd, "andrea")) { printf("andrea ok=%d (%.0f,%.0f)\n", PS.ok, PS.x / T, PS.y / T); if (PS.ok) { S.px = PS.x + 24; S.py = PS.y + 6; } continue; }
        else if (!strcmp(cmd, "fine")) { if (DLG.aperto) { DLG.i = DLG.npag - 1; DLG.n = 9999; } continue; }
        else if (!strcmp(cmd, "scheda")) { UI.tab = atoi(a1); continue; }
        else if (!strcmp(cmd, "foto")) {
            double a = ora(); gioco_disegna(&sopra, &sotto); tdis += ora() - a; ndis++; scatta(argv[3], a1);
            printf("foto %s  mappa=%s pos=(%.0f,%.0f) ora=%.2f monete=%d quest=%d/%d dlg=%d neg=%d\n", a1, M_ATT()->id, S.px / T, S.py / T, S.minuti / 60, S.monete, S.q_stato, S.q_passo, DLG.aperto, NEG.aperto);
            continue;
        }
        else if (!strcmp(cmd, "info")) { int k = 0; for (int i = 0; i < NSLOT; i++) if (S.inv[i].item >= 0) { printf("  slot %d: %s x%d\n", i, ITEMS[S.inv[i].item].id, S.inv[i].n); k++; } printf("  (%d slot pieni)\n", k); continue; }
        for (int p = 0; p < passi; p++) {
            Input q = p == 0 ? in : (Input){ 0 };
            q.tenuti = tenuti | q.giu; if (!strcmp(cmd, "cammina")) { q.ax = ax; q.ay = ay; }
            gioco_passo(1 / 60.f, &q);
            if (p % 6 == 0) { double a = ora(); gioco_disegna(&sopra, &sotto); tdis += ora() - a; ndis++; }
        }
    }
    printf("disegno medio: %.2f ms per fotogramma (%d fotogrammi)\n", ndis ? tdis / ndis * 1000 : 0, ndis);
    return 0;
}
