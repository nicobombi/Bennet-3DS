#!/bin/sh
# La Valle dei Germogli (Linux / macOS): compila, crea il .cia e mostra il QR per FBI.
# Servono devkitPro (3ds-dev), makerom e, facoltativo, bannertool nel PATH; python3 per il server.
set -e
cd "$(dirname "$0")"
make -j4
BANNER=""
if command -v bannertool >/dev/null 2>&1; then bannertool makebanner -i cia/banner.png -a cia/banner.wav -o build/banner.bnr && BANNER="-banner build/banner.bnr"; else echo "Nota: senza bannertool il banner del menu HOME resta vuoto."; fi
command -v makerom >/dev/null 2>&1 || { echo "Manca makerom: https://github.com/3DSGuy/Project_CTR/releases"; exit 1; }
rm -f ValleDeiGermogli.cia
makerom -f cia -o ValleDeiGermogli.cia -elf ValleDeiGermogli.elf -rsf cia/app.rsf -icon ValleDeiGermogli.smdh $BANNER -exefslogo -target t
echo "Creato ValleDeiGermogli.cia"
python3 strumenti/servi_qr.py
