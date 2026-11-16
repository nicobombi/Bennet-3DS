/* atmosfera come nella versione per PC: vento e raffiche, nuvole, pollini e petali, scie di vento, lucciole, fumo */
#ifndef VDG_ATMO_H
#define VDG_ATMO_H
#include "comune.h"

#define N_MOTE 160
#define N_SCIA 12
#define N_LUCC 40
typedef struct { float x, y, vy, drag, bob, ph, spin, life, max; uint16_t c; bool petalo; } Polline;
typedef struct { float x, y, len, amp, fr, ph, life, max; int dir; bool ricciolo; } Scia;
typedef struct { float x, y, vx, vy, ph, life, max, blink, off; } Lucciola;
typedef struct { float x, y, sp; int m; } Nuvola;
typedef struct {
    float wt, gust, forza, windT, fxT; int dir;
    Polline p[N_MOTE]; int np;
    Scia sc[N_SCIA]; int ns;
    Lucciola l[N_LUCC]; int nl;
    Nuvola nv[6];
    uint8_t *maschera[3]; int mw[3], mh[3];
    float notte, giorno, oro;          /* aggiornati da atmo_passo */
} Atmo;
extern Atmo A;

uint32_t hash3(int a, int b, int s);
float hashf(int a, int b, int s);
float ah(int x, int y, int k);
void atmo_inizia(void);
void atmo_entra(float px, float py);                    /* cambio mappa: nuvole intorno al personaggio, niente particelle */
void atmo_passo(float dt, int camx, int camy, bool interno, float notte, float ora);
void atmo_mondo(Sup *s, int camx, int camy);           /* pollini, petali e scie: prima della luce */
void atmo_lucciole(Sup *s, int camx, int camy);        /* puntini delle lucciole: dopo la luce */
float lucciola_luce(const Lucciola *f);
void fumo_disegna(Sup *s, int camx, int camy, float ox, float oy, int sd, int denso, float t);
#endif
