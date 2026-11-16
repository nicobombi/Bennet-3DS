/* Valle dei Germogli · versione per New Nintendo 3DS
   Tutto è disegnato in software su due superfici RGB565 (sopra 400×240, sotto 320×240); il livello di piattaforma
   (main_3ds.c o main_pc.c) le copia sullo schermo e fornisce i tasti. */
#ifndef VDG_COMUNE_H
#define VDG_COMUNE_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define SW 400
#define SH 240
#define BW 320
#define BH 240
#define T 16
#define KEY565 0xF81F

/* ---------- superficie e disegno ---------- */
typedef struct { uint16_t *px; int w, h; } Sup;
typedef struct { const char *nome; int w, h; uint16_t *px; } Img;   /* RGB565, KEY565 = trasparente */

static inline uint16_t rgb(int r, int g, int b) { return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)); }
void sup_riempi(Sup *s, uint16_t c);
void sup_rett(Sup *s, int x, int y, int w, int h, uint16_t c);
void sup_rett_a(Sup *s, int x, int y, int w, int h, uint16_t c, int a);          /* a = 0..256 */
void sup_somma(Sup *s, int x, int y, int w, int h, int r, int g, int b);         /* luce additiva (0..255 per canale) */
void sup_img(Sup *s, const Img *im, int x, int y, int a);                         /* a = 0..256 (256 = pieno) */
void sup_img_parte(Sup *s, const Img *im, int sx, int sy, int w, int h, int x, int y, int a);
void sup_img_scala(Sup *s, const Img *im, int sx, int sy, int w, int h, int x, int y, int z);
void sup_ellisse_ombra(Sup *s, int cx, int cy, int rx, int ry, int a);
void sup_cuore(Sup *s, int x, int y, uint16_t c1, uint16_t c2, int a);
void sup_stella(Sup *s, int x, int y, int sz, uint16_t c, int a);
uint16_t mix565(uint16_t a, uint16_t b, int t);                                   /* t = 0..256 verso b */

/* testo: UTF-8, carattere pixel alto 10 */
int testo(Sup *s, int x, int y, const char *t, uint16_t c);                       /* ritorna la larghezza */
int testo_ombra(Sup *s, int x, int y, const char *t, uint16_t c, uint16_t ombra);
int testo_largo(const char *t);
int testo_a_capo(Sup *s, int x, int y, int w, const char *t, uint16_t c, int maxch); /* ritorna l'altezza; maxch = caratteri da mostrare (-1 tutti) */
int testo_righe(int w, const char *t);
int testo_grande(Sup *s, int x, int y, const char *t, uint16_t c, uint16_t ombra, int z);

/* ---------- risorse (romfs) ---------- */
typedef struct { uint16_t w, h; uint32_t off; } SprInfo;
typedef struct {
    int ntiles; uint16_t tpal[256]; uint8_t *tiles;           /* ntiles × 256 indici */
    int nspr; uint16_t spal[256]; SprInfo *spr; uint8_t *sdat;
    int nimg; Img *img;
    int nitem;
    /* animazioni del terreno (anim.dat) */
    int gN, gF; uint16_t gpal[256]; uint8_t glut[256]; uint8_t *gfr;      /* erba: gF fotogrammi gN×gN, glut = indice del tile «cotto» */
    uint8_t *tflag;                                                       /* per tile: bit 0 = contiene erba che si muove */
    int nlaghi; struct Lago { int mappa, x, y, w, h, F; float fps; uint16_t pal[256]; uint8_t *fr; } laghi[4];
    int nfumi; struct Fumo { int mappa, denso, x, y, sd; } fumi[32];
} Risorse;
extern Risorse R;
bool risorse_carica(const char *base);
const Img *img(const char *nome);

/* ---------- oggetti dell'inventario ---------- */
typedef struct { char id[16]; char nome[40]; int max; int frames; char lore[160]; const Img *icona; const Img *fr[12]; } ItemDef;
extern ItemDef ITEMS[32];
int item_cerca(const char *id);

/* ---------- mappe ---------- */
typedef struct { int16_t x, y, base; uint16_t s; uint8_t chioma; } Ist;
typedef struct { int x, y, w, h, to, ax, ay; } Passo;
typedef struct { int x, y, r; uint8_t cr, cg, cb, a, interna; } Luce;
typedef struct { int x, y, w, h, interna; } Vetro;
typedef struct { int k, x, y; } NpcPos;
typedef struct { int x, y, item; } Racc;
typedef struct {
    char id[24], nome[48]; int interno, sotterraneo, W, H, spx, spy;
    uint16_t *tmap; uint8_t *solid, *ter;
    int nist; Ist *ist; int *riga;            /* riga[r] = primo oggetto con base >= r*T; maxsu = quanto sporgono in alto */
    int maxsu, maxgiu;
    int npassi; Passo *passi; int nluci; Luce *luci; int nvetri; Vetro *vetri; int ndiurne; int16_t *diurne;
    int nnpc; NpcPos *npc; int nracc; Racc *racc;
    int mw, mh; uint16_t *mini;
} Mappa;
extern Mappa MAPPE[8];
extern int NMAPPE;

/* ---------- input ---------- */
typedef struct {
    uint32_t giu, tenuti;          /* tasti appena premuti / tenuti (bit TS_*) */
    float ax, ay;                  /* levetta o croce: -1..1 */
    bool tocco, tocco_nuovo; int tx, ty;
} Input;
enum { TS_A = 1, TS_B = 2, TS_X = 4, TS_Y = 8, TS_L = 16, TS_R = 32, TS_START = 64, TS_SELECT = 128, TS_SU = 256, TS_GIU = 512, TS_SX = 1024, TS_DX = 2048 };

/* ---------- gioco (gioco.c) ---------- */
void gioco_inizia(void);
void gioco_passo(float dt, const Input *in);
void gioco_disegna(Sup *sopra, Sup *sotto);
bool gioco_esci(void);
void gioco_salva_ora(void);

/* salvataggio: il livello di piattaforma dice dove */
extern char PERCORSO_SALVA[256];

/* utilità */
static inline int clampi(int v, int a, int b) { return v < a ? a : v > b ? b : v; }
static inline float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
uint32_t rnd(void);
float rndf(void);
#endif
