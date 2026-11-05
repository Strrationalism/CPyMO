/* CPyMO for NDS 1.2 — original bounded FVDS elementary bit reader.
 * FVDS video bit words are LITTLE-ENDIAN 16-bit, consumed MSB-first.
 * It is NOT a VLC, inverse DCT, or movie picture renderer.
 * Single-thread, caller-owned memory; never touches FAT, IRQ, VRAM or ARM7.
 */
#ifndef CPYMO_NDS_FVDS_BITS_H
#define CPYMO_NDS_FVDS_BITS_H
#include <stdint.h>
#include <stddef.h>

typedef size_t (*nds_fvds_bit_pull)(void *context,void *dst,size_t maximum);

typedef struct {
 nds_fvds_bit_pull pull;
 void *context;
 uint8_t bytes[512];
 uint32_t total_bytes, fetched_bytes, consumed_bits;
 size_t available, at;
 uint16_t word;
 unsigned remaining_bits, failed;
} nds_fvds_bits;

/* Size must be even and bounded to a single FVDS picture payload.
 * Reader can only consume its own video bytes, never the following audio. */
int nds_fvds_bits_init(nds_fvds_bits *b,nds_fvds_bit_pull pull,
                       void *context,uint32_t picture_bytes);
/* Return 1 on complete success, 0 on invalid request or short input.
 * Reads 1..16 bits into *value. The caller must stop after failure. */
int nds_fvds_bits_read(nds_fvds_bits *b,unsigned count,uint32_t *value);
/* Align past remaining low bits of the current 16-bit storage word;
 * no read-ahead and no scanning for magic values. */
int nds_fvds_bits_align16(nds_fvds_bits *b);
#endif
