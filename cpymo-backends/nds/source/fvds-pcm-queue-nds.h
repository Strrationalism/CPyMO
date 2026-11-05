/* Original FVDS stereo PCM staging queue, AGPL-3.0-or-later.
 * Single-thread cooperative queue (caller-owned memory). This is NOT an
 * ARM7 shared-memory or hardware sound driver. Synchronization and cache
 * management are the responsibility of a future audio transport.
 */
#ifndef CPYMO_NDS_FVDS_PCM_QUEUE_H
#define CPYMO_NDS_FVDS_PCM_QUEUE_H
#include <stddef.h>
#include <stdint.h>

/* A signed stereo PCM frame contains 2 x int16_t = 4 bytes. */
typedef struct {
 int16_t *samples;
 size_t capacity; /* stereo FRAME count, not sample count */
 size_t read_at, write_at, queued, high_watermark;
 uint64_t produced, consumed, underflows, overflows;
} nds_fvds_pcm_queue;

/* Caller MUST supply at least 2*capacity int16_t slots. */
int nds_fvds_pcm_queue_init(nds_fvds_pcm_queue *q,int16_t *memory,
                            size_t capacity);
/* All-or-nothing push. Return zero if the queue is too full; no partial
 * sample write, no mutation of read or write indices on failure. */
int nds_fvds_pcm_queue_push(nds_fvds_pcm_queue *q,
                            const int16_t *interleaved,size_t frames);
/* Consume exactly frames stereo frames into dst; copy any available samples
 * and fill the shortfall with silence. Returns number of REAL frames.
 * For diagnostic purposes underflows counts requested missing frames. */
size_t nds_fvds_pcm_queue_drain(nds_fvds_pcm_queue *q,
                                int16_t *dst,size_t frames);
void nds_fvds_pcm_queue_reset(nds_fvds_pcm_queue *q);
#endif
