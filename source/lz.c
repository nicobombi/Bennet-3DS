#include "lz.h"
static uint32_t rd32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
size_t lz_dimensione(const uint8_t *src, size_t n) {
    if (n < 8 || src[0] != 'V' || src[1] != 'L' || src[2] != 'Z' || src[3] != '1') return 0;
    return rd32(src + 4);
}
size_t lz_decomprimi(const uint8_t *src, size_t n, uint8_t *out, size_t cap) {
    size_t tot = lz_dimensione(src, n), i = 8, o = 0;
    if (!tot || tot > cap) return 0;
    while (o < tot && i < n) {
        uint8_t flag = src[i++];
        for (int b = 0; b < 8 && o < tot && i < n; b++, flag >>= 1) {
            if (flag & 1) {
                if (i + 3 > n) return 0;
                size_t off = src[i] | (src[i + 1] << 8), len = (size_t)src[i + 2] + 4; i += 3;
                if (off == 0 || off > o || o + len > tot) return 0;
                uint8_t *d = out + o; const uint8_t *s = d - off;
                for (size_t k = 0; k < len; k++) d[k] = s[k];
                o += len;
            } else out[o++] = src[i++];
        }
    }
    return o == tot ? tot : 0;
}
