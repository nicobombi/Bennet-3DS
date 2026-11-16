/* LZ semplice (stile LZSS): un byte di flag ogni 8 elementi; bit 1 = copia (offset 16 bit, lunghezza 8 bit + 4), bit 0 = byte letterale.
   Formato del file: "VLZ1" + dimensione decompressa (u32 little endian) + flusso. */
#ifndef VDG_LZ_H
#define VDG_LZ_H
#include <stdint.h>
#include <stddef.h>
/* restituisce la dimensione decompressa (0 se il file non è valido); out deve essere grande abbastanza */
size_t lz_dimensione(const uint8_t *src, size_t n);
size_t lz_decomprimi(const uint8_t *src, size_t n, uint8_t *out, size_t cap);
#endif
