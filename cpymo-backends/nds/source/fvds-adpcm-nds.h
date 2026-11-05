/* CPyMO 1.2 NDS/DSi: original, allocation-free FVDS ADPCM block decoder.
 * ADPCM waveform reconstruction uses the documented IMA-style quantizer.
 * This contains no transplanted FastVideoDSPlayer source or ARM assembly.
 * Decoder ONLY: does not activate audio hardware, Maxmod, IPC or DMA.
 */
#ifndef CPYMO_NDS_FVDS_ADPCM_H
#define CPYMO_NDS_FVDS_ADPCM_H
#include <stddef.h>
#include <stdint.h>
enum { NDS_FVDS_ADPCM_BLOCK_BYTES=132, NDS_FVDS_ADPCM_PAIR_BYTES=264,
       NDS_FVDS_ADPCM_SAMPLES_PER_BLOCK=256,
       NDS_FVDS_ADPCM_INTERLEAVED_SAMPLES=512 };
/* Decode one independently-reset L/R pair (264 bytes) into 256 signed
 * stereo PCM frames (L0,R0,L1,R1,...). Pass at least 512 int16_t slots.
 * Return 256 output frames or a negative error. No partial output on
 * malformed headers (invalid step index). */
int nds_fvds_adpcm_pair(const uint8_t *in, size_t bytes,
                       int16_t *out_interleaved, size_t out_samples);
#endif
