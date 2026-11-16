#!/usr/bin/env python3
"""Offre ValleDeiGermogli.cia a FBI sulla rete locale e apre nel browser la pagina con il codice QR (solo libreria standard)."""
import http.server, socket, os, sys, webbrowser, functools
QUI = os.path.dirname(os.path.abspath(__file__)); RADICE = os.path.dirname(QUI); NOME = 'ValleDeiGermogli.cia'
CIA = os.path.join(RADICE, NOME)
if not os.path.exists(CIA): sys.exit('Non trovo ' + NOME + ': crea prima il .cia (crea_cia.sh).')
def ip_locale():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try: s.connect(('10.255.255.255', 1)); return s.getsockname()[0]   # nessun pacchetto viene spedito
    except OSError: return '127.0.0.1'
    finally: s.close()
porta = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
class Gestore(http.server.BaseHTTPRequestHandler):
    def _invia(self, corpo, tipo, testa=False):
        self.send_response(200); self.send_header('Content-Type', tipo); self.send_header('Content-Length', str(len(corpo))); self.end_headers()
        if not testa: self.wfile.write(corpo)
    def _gestisci(self, testa):
        p = self.path.split('?')[0]
        if p == '/' + NOME:
            n = os.path.getsize(CIA); self.send_response(200); self.send_header('Content-Type', 'application/octet-stream'); self.send_header('Content-Length', str(n)); self.end_headers()
            if testa: return
            print(f'{self.client_address[0]} scarica il gioco...', flush=True)
            with open(CIA, 'rb') as f:
                while True:
                    b = f.read(1 << 16)
                    if not b: break
                    self.wfile.write(b)
            print('Invio completato: conferma l\'installazione su FBI.', flush=True)
        elif p == '/':
            html = open(os.path.join(QUI, 'pagina_qr.html'), encoding='utf-8').read().replace('%%URL%%', URL)
            self._invia(html.encode('utf-8'), 'text/html; charset=utf-8', testa)
        elif p == '/qr.js': self._invia(open(os.path.join(QUI, 'qr.js'), 'rb').read(), 'application/javascript', testa)
        else: self.send_error(404)
    def do_GET(self): self._gestisci(False)
    def do_HEAD(self): self._gestisci(True)
    def log_message(self, *a): pass
srv = None
for p in range(porta, porta + 20):
    try: srv = http.server.ThreadingHTTPServer(('0.0.0.0', p), Gestore); porta = p; break
    except OSError: continue
if not srv: sys.exit('Nessuna porta libera.')
URL = f'http://{ip_locale()}:{porta}/{NOME}'
print('Indirizzo per FBI:', URL); print('Apro la pagina con il codice QR... (Ctrl+C per chiudere quando FBI ha finito)')
webbrowser.open(f'http://localhost:{porta}/')
try: srv.serve_forever()
except KeyboardInterrupt: pass
