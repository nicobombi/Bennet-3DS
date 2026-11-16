/* generato da strumenti/dati_h.py: non modificare a mano */
#ifndef VDG_DATI_H
#define VDG_DATI_H
__attribute__((unused)) static const float SKY[][4] = {{0.0f,0.3f,0.33f,0.56f},{4.6f,0.3f,0.33f,0.56f},{5.5f,0.33f,0.35f,0.59f},{6.0f,0.4f,0.4f,0.63f},{6.4f,0.52f,0.47f,0.66f},{6.8f,0.7f,0.58f,0.64f},{7.2f,0.86f,0.7f,0.64f},{7.7f,0.96f,0.84f,0.72f},{8.3f,1.0f,0.95f,0.87f},{9.0f,1.0f,1.0f,1.0f},{16.4f,1.0f,1.0f,1.0f},{17.6f,1.0f,0.91f,0.76f},{18.6f,0.96f,0.74f,0.6f},{19.5f,0.66f,0.54f,0.68f},{20.4f,0.44f,0.43f,0.66f},{21.3f,0.32f,0.35f,0.58f},{24.0f,0.3f,0.33f,0.56f}};
#define NSKY 17
__attribute__((unused)) static const float ZB_PERCORSO[][2] = {{3.3f,-3.7f},{2.4f,-2.5f},{1.2f,-1.7f},{0.2f,-1.0f},{0.0f,0.0f}};
#define NZB 5
__attribute__((unused)) static const char *Q_TITOLO = "La Shota di Siro", *Q_DA = "Siro", *Q_DOVE = "Grate del parcheggio", *Q_PREMIO = "???";
__attribute__((unused)) static const char *Q_DESCR[] = {"A Siro è caduta la Shota tra le grate del parcheggio. Ha provato a riprenderla, ma il braccio non arriva.","Dice che sotto il parcheggio c'è un passaggio che porta ai condotti. Lui da solo non ha il coraggio di scendere."};
#define NQDESCR 2
__attribute__((unused)) static const char *Q_PASSI[] = {"Trova un modo per scendere sotto il parcheggio","Recupera la Shota nei condotti","Riporta la Shota a Siro"};
#define NQPASSI 3
__attribute__((unused)) static const char *MSG_DA = "La Valle dei Germogli", *MSG_TITOLO = "Benvenuto!", *MSG_TESTO = "Benvenuto nella Valle. Qui troverai le lettere degli abitanti, gli avvisi e le ricompense.\nIl diario è sempre qui, sullo schermo in basso.";
__attribute__((unused)) static const int RIV_SHOTA = 150, RIV_ORO = 2500, ACQ_SHOTA = 50, ACQ_ORO = 1000;
__attribute__((unused)) static const char *DICE_SHOTA = "Mmh. Roba fatta a mano, si sente. Cinquanta monete, prendere o lasciare.", *DICE_ORO = "...Per tutte le lune di questo parco. È autentica. Mille monete, e non fare domande.";
__attribute__((unused)) static const char *FRASI_ZAMBU[] = {"Oggi non ho spento la Playstation"};
#define NFRASI_ZAMBU 1
__attribute__((unused)) static const char *FRASI_SIRO[] = {"Bvo... Mi è caduta la Shota"};
#define NFRASI_SIRO 1
__attribute__((unused)) static const char *FRASI_ANDREA[] = {"Ciao! Facciamo il solito giro: lui non si stanca mai.","Piano, piano! Ogni lampione va annusato, per lui.","Se lo accarezzi ti fa le feste, è buonissimo.","Il parco al mattino è il momento più bello, vero?"};
#define NFRASI_ANDREA 4
__attribute__((unused)) static const int ZB_BRACE_GIU[2] = {12,22}, ZB_BRACE_SU[2] = {4,14};
#endif
