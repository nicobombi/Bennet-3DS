/* lettura delle risorse dal romfs: tutti i file sono compressi con lz (vedi lz.h) */
#include "comune.h"
#include "lz.h"

Risorse R;
Mappa MAPPE[8];
int NMAPPE;
ItemDef ITEMS[32];

static uint8_t *carica_file(const char *base, const char *nome, size_t *dim) {
    char p[300]; snprintf(p, sizeof p, "%s%s", base, nome);
    FILE *f = fopen(p, "rb"); if (!f) { printf("manca %s\n", p); return NULL; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *c = malloc(n); if (!c) { fclose(f); return NULL; }
    if (fread(c, 1, n, f) != (size_t)n) { fclose(f); free(c); return NULL; }
    fclose(f);
    size_t d = lz_dimensione(c, n); uint8_t *o = d ? malloc(d) : NULL;
    if (!o || lz_decomprimi(c, n, o, d) != d) { printf("file rovinato %s\n", p); free(c); free(o); return NULL; }
    free(c); *dim = d; return o;
}

/* lettore sequenziale (little endian, senza accessi non allineati) */
typedef struct { const uint8_t *p, *fine; } Let;
static int u8(Let *l) { return l->p < l->fine ? *l->p++ : 0; }
static int u16(Let *l) { int a = u8(l); return a | (u8(l) << 8); }
static int s16(Let *l) { return (int16_t)u16(l); }
static uint32_t u32(Let *l) { uint32_t a = u16(l); return a | ((uint32_t)u16(l) << 16); }
static void str8(Let *l, char *d, int cap) { int n = u8(l); for (int i = 0; i < n; i++) { int c = u8(l); if (i < cap - 1) d[i] = c; } d[n < cap - 1 ? n : cap - 1] = 0; }
static void str16(Let *l, char *d, int cap) { int n = u16(l); for (int i = 0; i < n; i++) { int c = u8(l); if (i < cap - 1) d[i] = c; } d[n < cap - 1 ? n : cap - 1] = 0; }
static void *copia(Let *l, size_t n) { void *d = malloc(n ? n : 1); memcpy(d, l->p, n); l->p += n; return d; }
static bool magia(Let *l, const char *m) { bool ok = memcmp(l->p, m, 4) == 0; l->p += 4; return ok; }

const Img *img(const char *nome) {
    for (int i = 0; i < R.nimg; i++) if (!strcmp(R.img[i].nome, nome)) return &R.img[i];
    return NULL;
}
int item_cerca(const char *id) { for (int i = 0; i < R.nitem; i++) if (!strcmp(ITEMS[i].id, id)) return i; return -1; }

bool risorse_carica(const char *base) {
    size_t n; uint8_t *b; Let l;
    /* tile del terreno: restano indici a 8 bit in memoria (circa 10 MB) */
    if (!(b = carica_file(base, "tiles.dat", &n))) return false;
    l = (Let){ b, b + n }; if (!magia(&l, "VDGT")) return false;
    R.ntiles = u32(&l); for (int i = 0; i < 256; i++) R.tpal[i] = u16(&l);
    R.tiles = b; memmove(b, l.p, (size_t)R.ntiles * 256);           /* riusa lo stesso blocco */
    /* sprite degli oggetti */
    if (!(b = carica_file(base, "sprites.dat", &n))) return false;
    l = (Let){ b, b + n }; if (!magia(&l, "VDGS")) return false;
    R.nspr = u32(&l); for (int i = 0; i < 256; i++) R.spal[i] = u16(&l);
    R.spr = malloc(sizeof(SprInfo) * R.nspr);
    for (int i = 0; i < R.nspr; i++) { R.spr[i].w = u16(&l); R.spr[i].h = u16(&l); R.spr[i].off = u32(&l); }
    size_t resto = l.fine - l.p; R.sdat = malloc(resto); memcpy(R.sdat, l.p, resto); free(b);
    /* immagini dei personaggi, icone e oggetti */
    if (!(b = carica_file(base, "pers.dat", &n))) return false;
    l = (Let){ b, b + n }; if (!magia(&l, "VDGP")) return false;
    R.nimg = u32(&l); R.img = calloc(R.nimg, sizeof(Img));
    for (int i = 0; i < R.nimg; i++) {
        char nm[64]; str8(&l, nm, sizeof nm); Img *im = &R.img[i]; im->nome = strdup(nm);
        im->w = u16(&l); im->h = u16(&l); im->px = malloc(im->w * im->h * 2);
        for (int k = 0; k < im->w * im->h; k++) im->px[k] = u16(&l);
    }
    free(b);
    /* oggetti dell'inventario */
    if (!(b = carica_file(base, "oggetti.dat", &n))) return false;
    l = (Let){ b, b + n }; if (!magia(&l, "VDGI")) return false;
    R.nitem = u32(&l); if (R.nitem > 32) R.nitem = 32;
    for (int i = 0; i < R.nitem; i++) {
        ItemDef *it = &ITEMS[i]; str8(&l, it->id, sizeof it->id); str8(&l, it->nome, sizeof it->nome);
        it->max = u16(&l); it->frames = u8(&l); str16(&l, it->lore, sizeof it->lore);
        char nm[64]; snprintf(nm, sizeof nm, "item_%s", it->id); it->icona = img(nm);
        for (int f = 0; f < it->frames && f < 12; f++) { snprintf(nm, sizeof nm, "item_%s_%d", it->id, f); it->fr[f] = img(nm); }
    }
    free(b);
    /* mappe */
    if (!(b = carica_file(base, "mappe.dat", &n))) return false;
    l = (Let){ b, b + n }; if (!magia(&l, "VDGM")) return false;
    NMAPPE = u32(&l); if (NMAPPE > 8) NMAPPE = 8;
    for (int m = 0; m < NMAPPE; m++) {
        Mappa *M = &MAPPE[m]; memset(M, 0, sizeof *M);
        str8(&l, M->id, sizeof M->id); str8(&l, M->nome, sizeof M->nome);
        M->interno = u8(&l); M->sotterraneo = u8(&l); M->W = u16(&l); M->H = u16(&l); M->spx = s16(&l); M->spy = s16(&l);
        int WH = M->W * M->H;
        M->tmap = malloc(WH * 2); for (int i = 0; i < WH; i++) M->tmap[i] = u16(&l);
        M->solid = copia(&l, WH); M->ter = copia(&l, WH);
        M->nist = u32(&l); M->ist = malloc(sizeof(Ist) * (M->nist ? M->nist : 1));
        M->maxsu = 0; M->maxgiu = 0;
        for (int i = 0; i < M->nist; i++) {
            Ist *o = &M->ist[i]; o->s = u16(&l); o->x = s16(&l); o->y = s16(&l); o->base = s16(&l); o->chioma = u8(&l);
            const SprInfo *si = &R.spr[o->s];
            if (o->base - o->y > M->maxsu) M->maxsu = o->base - o->y;
            if (o->y + si->h - o->base > M->maxgiu) M->maxgiu = o->y + si->h - o->base;
        }
        /* indice per righe di caselle: gli oggetti sono ordinati per base */
        int righe = M->H + 2; M->riga = malloc(sizeof(int) * (righe + 1));
        for (int r = 0, k = 0; r <= righe; r++) { while (k < M->nist && M->ist[k].base < r * T) k++; M->riga[r] = k; }
        M->npassi = u16(&l); M->passi = malloc(sizeof(Passo) * (M->npassi + 1));
        for (int i = 0; i < M->npassi; i++) { Passo *p = &M->passi[i]; p->x = u16(&l); p->y = u16(&l); p->w = u16(&l); p->h = u16(&l); p->to = u8(&l); p->ax = s16(&l); p->ay = s16(&l); }
        M->nluci = u16(&l); M->luci = malloc(sizeof(Luce) * (M->nluci + 1));
        for (int i = 0; i < M->nluci; i++) { Luce *c = &M->luci[i]; c->x = s16(&l); c->y = s16(&l); c->r = u16(&l); c->cr = u8(&l); c->cg = u8(&l); c->cb = u8(&l); c->a = u8(&l); c->interna = u8(&l); }
        M->nvetri = u16(&l); M->vetri = malloc(sizeof(Vetro) * (M->nvetri + 1));
        for (int i = 0; i < M->nvetri; i++) { Vetro *v = &M->vetri[i]; v->x = s16(&l); v->y = s16(&l); v->w = u8(&l); v->h = u8(&l); v->interna = u8(&l); }
        M->ndiurne = u16(&l); M->diurne = malloc(4 * (M->ndiurne + 1));
        for (int i = 0; i < M->ndiurne * 2; i++) M->diurne[i] = s16(&l);
        M->nnpc = u16(&l); M->npc = malloc(sizeof(NpcPos) * (M->nnpc + 1));
        for (int i = 0; i < M->nnpc; i++) { M->npc[i].k = u8(&l); M->npc[i].x = u16(&l); M->npc[i].y = u16(&l); }
        M->nracc = u16(&l); M->racc = malloc(sizeof(Racc) * (M->nracc + 1));
        for (int i = 0; i < M->nracc; i++) { M->racc[i].x = u16(&l); M->racc[i].y = u16(&l); M->racc[i].item = u8(&l); }
        M->mw = u16(&l); M->mh = u16(&l); M->mini = malloc(M->mw * M->mh * 2);
        for (int i = 0; i < M->mw * M->mh; i++) M->mini[i] = u16(&l);
    }
    free(b);
    /* animazioni: erba al vento, acqua dei laghetti, fumo */
    if ((b = carica_file(base, "anim.dat", &n))) {
        l = (Let){ b, b + n };
        if (magia(&l, "VDGA")) {
            R.gN = u16(&l); R.gF = u8(&l); int np = u16(&l);
            for (int i = 0; i < np; i++) R.gpal[i] = u16(&l);
            for (int i = 0; i < np; i++) R.glut[i] = u8(&l);
            size_t fs = (size_t)R.gN * R.gN; R.gfr = malloc(fs * R.gF); memcpy(R.gfr, l.p, fs * R.gF); l.p += fs * R.gF;
            for (int k = 1; k < R.gF; k++) for (size_t i = 0; i < fs; i++) R.gfr[k * fs + i] ^= R.gfr[i];
            /* quali tile contengono colori dell'erba (per saltare subito gli altri) */
            uint8_t inerba[256] = { 0 }; for (int i = 0; i < np; i++) inerba[R.glut[i]] = 1;
            R.tflag = calloc(R.ntiles, 1);
            for (int t = 0; t < R.ntiles; t++) { const uint8_t *p = R.tiles + (size_t)t * 256; for (int i = 0; i < 256; i++) if (inerba[p[i]]) { R.tflag[t] |= 1; break; } }
            R.nlaghi = u16(&l);
            for (int k = 0; k < R.nlaghi && k < 4; k++) {
                struct Lago *L = &R.laghi[k]; L->mappa = u8(&l); L->x = s16(&l); L->y = s16(&l); L->w = u16(&l); L->h = u16(&l); L->F = u8(&l); L->fps = u8(&l) / 10.f;
                for (int i = 0; i < 256; i++) L->pal[i] = u16(&l);
                l.p += 256;                                            /* corrispondenza verso i tile: non serve più */
                size_t fs2 = (size_t)L->w * L->h; L->fr = malloc(fs2 * L->F); memcpy(L->fr, l.p, fs2 * L->F); l.p += fs2 * L->F;
                for (int f = 1; f < L->F; f++) for (size_t i = 0; i < fs2; i++) L->fr[f * fs2 + i] ^= L->fr[i];
            }
            R.nfumi = u16(&l);
            for (int k = 0; k < R.nfumi && k < 32; k++) { R.fumi[k].mappa = u8(&l); R.fumi[k].denso = u8(&l); R.fumi[k].x = s16(&l); R.fumi[k].y = s16(&l); R.fumi[k].sd = u16(&l); }
            if (R.nfumi > 32) R.nfumi = 32;
        }
        free(b);
    }
    return true;
}
