/* MOSTRO ENERGIA: la lattina disegnata pixel per pixel (12 × 22), modificabile qui.
   Ogni carattere è un pixel: '.' = trasparente, gli altri sono i colori di PAL. */
#include "stato.h"

static const char *RIGHE[] = {
    "..kkkkkkkk..",
    ".kccccdddck.",
    "keccddfffcck",
    "kkeeedddccfk",
    "kkffkkfkkkfk",
    "kkffkkfkkkfk",
    "kkGgkGGkggfk",
    "kkfGkkgkkgfk",
    "kkfGkkGkkGfk",
    "kkfgkkGkkGfk",
    "kkfgkkgkkGfk",
    "kkfgkkGkkgfk",
    "kkfGkkGkkgfk",
    "kkfGkkgkkGfk",
    "kkfGkkgkkGfk",
    "kkfGkkGkkgfk",
    "kkffkkGkkkfk",
    "kkffkkfkkkfk",
    "keffkkfkkkck",
    "nkeecceccckn",
    ".nkkkkkkkkn.",
    "..nnnnnnnn.."
};
static const struct { char k; uint8_t r, g, b; } PAL[] = {
    { 'k', 0, 0, 0 }, { 'c', 159, 159, 159 }, { 'd', 112, 112, 112 }, { 'e', 200, 200, 200 },
    { 'f', 34, 34, 34 }, { 'n', 94, 94, 94 }, { 'G', 151, 235, 4 }, { 'g', 139, 204, 5 },
};
static Img ICONA = { "item_mostro", 0, 0, NULL };
int IT_MOSTRO = -1;

void mostro_registra(void) {
    int H = sizeof RIGHE / sizeof RIGHE[0], W = (int)strlen(RIGHE[0]);
    ICONA.w = W; ICONA.h = H; ICONA.px = malloc(W * H * 2);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        uint16_t c = KEY565;
        for (size_t i = 0; i < sizeof PAL / sizeof PAL[0]; i++) if (PAL[i].k == RIGHE[y][x]) c = rgb(PAL[i].r, PAL[i].g, PAL[i].b);
        ICONA.px[y * W + x] = c;
    }
    if (R.nitem >= 32) return;
    ItemDef *it = &ITEMS[R.nitem]; memset(it, 0, sizeof *it);
    snprintf(it->id, sizeof it->id, "mostro"); snprintf(it->nome, sizeof it->nome, "Mostro Energia");
    snprintf(it->lore, sizeof it->lore, "Lattina nera con tre graffi verdi. Una sorsata e le gambe vanno da sole: per 3 minuti corri più veloce.");
    it->max = 16; it->icona = &ICONA;
    IT_MOSTRO = R.nitem++;
}
