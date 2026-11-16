/* QR code minimale (modalità byte, correzione L, versioni 1-10): niente librerie esterne.
   qrMatrice(testo) → array di righe di booleani (true = modulo scuro), senza margine. */
function qrMatrice(testo) {
  const dati = Array.from(new TextEncoder().encode(testo));
  // [codeword totali, ec per blocco, [[numero blocchi, dati per blocco], ...]] per la correzione L
  const TAB = [null, [26, 7, [[1, 19]]], [44, 10, [[1, 34]]], [70, 15, [[1, 55]]], [100, 20, [[1, 80]]], [134, 26, [[1, 108]]],
    [172, 18, [[2, 68]]], [196, 20, [[2, 78]]], [242, 24, [[2, 97]]], [292, 30, [[2, 116]]], [346, 18, [[2, 68], [2, 69]]]];
  const ALL = [null, [], [6, 18], [6, 22], [6, 26], [6, 30], [6, 34], [6, 22, 38], [6, 24, 42], [6, 26, 46], [6, 28, 50]];
  let ver = 1;
  for (; ver <= 10; ver++) { const cap = TAB[ver][2].reduce((s, [n, d]) => s + n * d, 0); if (4 + (ver < 10 ? 8 : 16) + dati.length * 8 <= cap * 8) break; }
  if (ver > 10) throw new Error('testo troppo lungo per il QR');
  const [, ecn, gruppi] = TAB[ver], cap = gruppi.reduce((s, [n, d]) => s + n * d, 0);
  // flusso di bit
  const bit = []; const put = (v, n) => { for (let i = n - 1; i >= 0; i--) bit.push((v >>> i) & 1); };
  put(4, 4); put(dati.length, ver < 10 ? 8 : 16); for (const b of dati) put(b, 8);
  put(0, Math.min(4, cap * 8 - bit.length)); while (bit.length % 8) bit.push(0);
  const cw = []; for (let i = 0; i < bit.length; i += 8) cw.push(parseInt(bit.slice(i, i + 8).join(''), 2));
  for (let p = 0xEC; cw.length < cap; p ^= 0xEC ^ 0x11) cw.push(p);
  // Reed-Solomon su GF(256), polinomio 0x11D
  const EXP = new Array(512), LOG = new Array(256); let x = 1;
  for (let i = 0; i < 255; i++) { EXP[i] = x; LOG[x] = i; x <<= 1; if (x & 256) x ^= 0x11D; }
  for (let i = 255; i < 512; i++) EXP[i] = EXP[i - 255];
  const mul = (a, b) => a && b ? EXP[LOG[a] + LOG[b]] : 0;
  let gen = [1];
  for (let i = 0; i < ecn; i++) { const g = new Array(gen.length + 1).fill(0); for (let j = 0; j < gen.length; j++) { g[j] ^= gen[j]; g[j + 1] ^= mul(gen[j], EXP[i]); } gen = g; }
  const rs = d => { const r = new Array(ecn).fill(0); for (const b of d) { const f = b ^ r.shift(); r.push(0); for (let j = 0; j < ecn; j++) r[j] ^= mul(gen[j + 1], f); } return r; };
  const blocchi = []; let k = 0;
  for (const [n, d] of gruppi) for (let i = 0; i < n; i++) { const b = cw.slice(k, k + d); k += d; blocchi.push([b, rs(b)]); }
  const fin = [], maxd = Math.max(...blocchi.map(b => b[0].length));
  for (let i = 0; i < maxd; i++) for (const [d] of blocchi) if (i < d.length) fin.push(d[i]);
  for (let i = 0; i < ecn; i++) for (const [, e] of blocchi) fin.push(e[i]);
  // matrice e moduli di funzione
  const N = ver * 4 + 17, M = [], F = [];
  for (let y = 0; y < N; y++) { M.push(new Array(N).fill(false)); F.push(new Array(N).fill(false)); }
  const set = (x, y, v) => { M[y][x] = !!v; F[y][x] = true; };
  for (let i = 0; i < N; i++) { set(6, i, i % 2 === 0); set(i, 6, i % 2 === 0); }
  const finder = (cx, cy) => { for (let dy = -4; dy <= 4; dy++) for (let dx = -4; dx <= 4; dx++) { const xx = cx + dx, yy = cy + dy; if (xx < 0 || yy < 0 || xx >= N || yy >= N) continue; const d = Math.max(Math.abs(dx), Math.abs(dy)); set(xx, yy, d !== 2 && d !== 4); } };
  finder(3, 3); finder(N - 4, 3); finder(3, N - 4);
  const al = ALL[ver];
  for (let i = 0; i < al.length; i++) for (let j = 0; j < al.length; j++) {
    if ((i === 0 && j === 0) || (i === 0 && j === al.length - 1) || (i === al.length - 1 && j === 0)) continue;
    for (let dy = -2; dy <= 2; dy++) for (let dx = -2; dx <= 2; dx++) set(al[i] + dx, al[j] + dy, Math.max(Math.abs(dx), Math.abs(dy)) !== 1);
  }
  const formato = mask => {
    const d = (1 << 3) | mask; let r = d; for (let i = 0; i < 10; i++) r = (r << 1) ^ ((r >>> 9) * 0x537);
    const b = ((d << 10) | r) ^ 0x5412, g = i => (b >>> i) & 1;
    for (let i = 0; i <= 5; i++) set(8, i, g(i));
    set(8, 7, g(6)); set(8, 8, g(7)); set(7, 8, g(8));
    for (let i = 9; i < 15; i++) set(14 - i, 8, g(i));
    for (let i = 0; i < 8; i++) set(N - 1 - i, 8, g(i));
    for (let i = 8; i < 15; i++) set(8, N - 15 + i, g(i));
    set(8, N - 8, 1);
  };
  formato(0);
  if (ver >= 7) {
    let r = ver; for (let i = 0; i < 12; i++) r = (r << 1) ^ ((r >>> 11) * 0x1F25);
    const b = (ver << 12) | r;
    for (let i = 0; i < 18; i++) { const v = (b >>> i) & 1, a = N - 11 + i % 3, c = Math.floor(i / 3); set(a, c, v); set(c, a, v); }
  }
  // dati a zig-zag
  let i = 0;
  for (let right = N - 1; right >= 1; right -= 2) {
    if (right === 6) right = 5;
    for (let v = 0; v < N; v++) for (let j = 0; j < 2; j++) {
      const xx = right - j, su = ((right + 1) & 2) === 0, yy = su ? N - 1 - v : v;
      if (!F[yy][xx] && i < fin.length * 8) { M[yy][xx] = ((fin[i >>> 3] >>> (7 - (i & 7))) & 1) === 1; i++; }
    }
  }
  const MASCHERE = [(x, y) => (x + y) % 2 === 0, (x, y) => y % 2 === 0, (x, y) => x % 3 === 0, (x, y) => (x + y) % 3 === 0,
    (x, y) => (Math.floor(x / 3) + Math.floor(y / 2)) % 2 === 0, (x, y) => x * y % 2 + x * y % 3 === 0,
    (x, y) => (x * y % 2 + x * y % 3) % 2 === 0, (x, y) => ((x + y) % 2 + x * y % 3) % 2 === 0];
  const applica = m => { for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) if (!F[y][x] && MASCHERE[m](x, y)) M[y][x] = !M[y][x]; };
  const penalita = () => {
    let p = 0, scuri = 0;
    for (let a = 0; a < 2; a++) for (let u = 0; u < N; u++) {
      let run = 1, riga = [];
      for (let v = 0; v < N; v++) { const c = a ? M[v][u] : M[u][v]; riga.push(c); if (v && c === riga[v - 1]) { run++; if (run === 5) p += 3; else if (run > 5) p++; } else run = 1; }
      const s = riga.map(c => c ? 1 : 0).join('');
      for (const pat of ['10111010000', '00001011101']) { let k = s.indexOf(pat); while (k >= 0) { p += 40; k = s.indexOf(pat, k + 1); } }
    }
    for (let y = 0; y < N - 1; y++) for (let x = 0; x < N - 1; x++) { const c = M[y][x]; if (c === M[y][x + 1] && c === M[y + 1][x] && c === M[y + 1][x + 1]) p += 3; }
    for (const r of M) for (const c of r) if (c) scuri++;
    return p + (Math.ceil(Math.abs(scuri * 20 - N * N * 10) / (N * N)) - 1) * 10;
  };
  let migliore = 0, minp = Infinity;
  for (let m = 0; m < 8; m++) { applica(m); formato(m); const p = penalita(); if (p < minp) { minp = p; migliore = m; } applica(m); }
  applica(migliore); formato(migliore);
  return M;
}
if (typeof module !== 'undefined') module.exports = { qrMatrice };
