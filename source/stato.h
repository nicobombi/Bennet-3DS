/* stato condiviso fra gioco.c (logica), mondo.c (schermo sopra) e ui.c (schermo sotto) */
#ifndef VDG_STATO_H
#define VDG_STATO_H
#include "comune.h"

#define NSLOT 36
#define NBARRA 9
#define FESTA_DUR 4.6f
enum { D_GIU, D_SX, D_DX, D_SU };                 /* stesso ordine delle righe dei fogli sprite */
enum { SCH_TITOLO, SCH_GIOCO };
enum { TAB_ZAINO, TAB_QUEST, TAB_MSG, TAB_MAPPA, TAB_OPZ, NTAB };
enum { Q_NO, Q_ATTIVA, Q_FATTA };
enum { NPC_SIRO = 1, NPC_ZAMBU = 2, NPC_ANDREA = 3 };

typedef struct { int8_t item; uint8_t n; } Slot;

/* azioni delle scelte nei dialoghi */
enum { A_CHIUDI, A_COMPRA, A_VENDI, A_SIRO_SI, A_SIRO_NO, A_SIRO_SCAMBIO, A_SIRO_SPECIALE, A_SIRO_FINE, A_ACCAREZZA };

typedef struct {
    /* salvato */
    int mappa; float px, py; int dir; float minuti; int giorno; int monete;
    Slot inv[NSLOT]; int mano;
    int q_stato, q_passo;
    uint8_t raccolti[8];                          /* un bit per oggetto a terra, per mappa */
    int8_t mag[16]; int nmag;                     /* quello che Zambu ha comprato e rivende */
    bool msg_letto;
    /* durante il gioco */
    int schermata; bool moving; float dist; float t;
    float festa;                                  /* secondi dall'inizio della festa (grande = niente festa) */
    float fade; int fade_to, fade_ax, fade_ay; bool fade_fatto; int passo_dentro;
    float amb[3], notte, fuori; bool amb_ok;
    float nome_luogo;                             /* timer del nome della mappa in alto */
    bool c_salvataggio;                           /* esiste un salvataggio (titolo) */
    int fps; int camx, camy;
} Stato;
typedef struct { float x, y, t; } Brillo;
extern Brillo BRILLI[4];
extern Stato S;

/* Zambu: arriva alle 22:30 da dietro la quercia, se ne va alle 6:00 */
typedef struct { bool c; int tx, ty; float pts[8][2], seg[8], L, s; int stato; float x, y; int dir; float passo, alpha; } Zambu;
enum { ZB_VIA, ZB_ARRIVA, ZB_FERMO, ZB_PARTE };
extern Zambu ZB;
typedef struct { bool c; int tx, ty; } Siro;
extern Siro SI;
/* Andrea e il barboncino */
typedef struct { float x, y, dist, guarda; int dir; bool fermo; } Cane;
typedef struct { bool ok, parla; float x, y, dist, pausa, vel, cuori; int dir, dx, dy, tx, ty; float scia[90][2]; int nscia; Cane cane; } Passeggio;
extern Passeggio PS;

/* dialogo da visual novel */
#define DLG_PAGINE 8
typedef struct { char t[40]; int a; } Scelta;
typedef struct {
    bool aperto; int chi; char nome[24];
    char pag[DLG_PAGINE][320]; int npag, i; float t; int n;   /* n = caratteri mostrati */
    Scelta sc[4]; int nsc, sel;
} Dialogo;
extern Dialogo DLG;
/* bancarella di Zambu (compra / vendi) */
typedef struct { bool aperto, vendita; int sel; int righe[16]; int prezzi[16]; int nrighe; char testa[120]; } Negozio;
extern Negozio NEG;

typedef struct { char t[96]; float vita; } Toast;
extern Toast TOAST[3];
typedef struct { int tab, sel, sel_msg; bool luci, vignetta, effetti, bagliore; float scorri; int titolo_sel; bool conferma_nuova; } Ui;
extern Ui UI;

/* gioco.c */
Mappa *M_ATT(void);
void toast(const char *fmt, ...);
int item_conta(int item);
bool item_dai(int item, int n);
bool item_togli(int item);
void azione(int a);
void dialogo_avanza(void);
void negozio_apri(bool vendita);
void negozio_esegui(int riga);
void negozio_chiudi(void);
void partita_nuova(void);
bool partita_carica(void);
void partita_salva(void);
bool npc_vicino(int *chi, float *x, float *y);
float ora_del_giorno(void);
const char *moscia(const char *s, char *buf, int cap);
void debug_entra(int m, int x, int y);
void gioco_chiedi_uscita(void);

/* mondo.c */
void mondo_disegna(Sup *s, bool titolo);
void mondo_entra(void);                           /* cambio mappa: azzera le cache */
bool sprite_opaco(int s, int x, int y);

/* ui.c */
void ui_disegna_sopra(Sup *s);                    /* orologio, avvisi, dialogo */
void ui_disegna_sotto(Sup *s);
bool ui_input(const Input *in);                   /* true = l'input è stato usato (niente movimento) */
void ui_titolo_input(const Input *in);
void ui_ritratto(Sup *s, int chi, int x, int y, int z);
#endif
