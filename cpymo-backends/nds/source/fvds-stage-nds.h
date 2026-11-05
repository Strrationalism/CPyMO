/* CPyMO NDS 1.2: original bounded FVDS frame/audio scheduling adapter.
 * Consumes validated FVDS packets and emits COMPRESSED VIDEO chunks plus
 * DECODED stereo PCM. A future video decoder / ARM7 producer owns callbacks.
 * No VRAM, DMA, ARM7, IRQ, audio hardware, FAT mounts or heap allocation.
 */
#ifndef CPYMO_NDS_FVDS_STAGE_H
#define CPYMO_NDS_FVDS_STAGE_H
#include "fvds-stream-nds.h"
#include "fvds-adpcm-nds.h"

enum nds_fvds_step_result {
 NDS_FVDS_STEP_ERROR = -1,
 NDS_FVDS_STEP_FRAME = 0,
 NDS_FVDS_STEP_EOF = 1,
 NDS_FVDS_STEP_SKIPPED = 2
};

/* Each callback returns 0=continue, +1=user skip, negative=error.
 * Chunks are ephemeral: callback MUST consume synchronously; it must NOT
 * retain pointers into stage. video_begin/video_chunk/video_end are required:
 * passing bytes alone is not video decoding. */
typedef struct {
 int (*poll)(void *ctx);
 int (*video_begin)(void *ctx, uint32_t frame, uint64_t pts_us);
 int (*video_chunk)(void *ctx, const uint8_t *data, size_t bytes);
 int (*video_end)(void *ctx, uint32_t frame);
 int (*audio_pcm)(void *ctx, const int16_t *interleaved, size_t frames);
} nds_fvds_stage_callbacks;

typedef struct {
 /* Deliberately caller-owned: buffers survive synchronous callbacks, no
  * per-frame allocation and only 512 + 264 + 1024 bytes of staging RAM. */
 uint8_t video[512], audio[264];
 int16_t pcm[512];
 uint32_t completed_frames;
 uint64_t audio_frames;
 uint32_t key_pictures, predicted_pictures;
} nds_fvds_stage;

/* Call exactly once per video frame. On B skip/error the current FVDS packet
 * is finished (seek to next packet) when possible, then return to caller.
 * The caller remains responsible for stopping audio, restoring VRAM/BGM,
 * closing FVDS and redrawing CPyMO. */
int nds_fvds_stage_step(nds_fvds_stream *file, nds_fvds_stage *stage,
                        const nds_fvds_stage_callbacks *callbacks, void *ctx);
#endif
