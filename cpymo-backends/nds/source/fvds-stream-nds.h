/* CPyMO NDS 1.2 experimental FVDS packet reader. AGPL-3.0-or-later.
 * Original container I/O implementation; NO FVDS video or audio decoder.
 * Does not take ownership of ARM7/FAT/VRAM/DMA/audio, or allocate memory.
 */
#ifndef CPYMO_NDS_FVDS_STREAM_H
#define CPYMO_NDS_FVDS_STREAM_H
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
 FILE *file;
 uint32_t bytes, frame_count, frame_index, keyframe_count;
 uint32_t data_offset, fps_num, fps_den;
 uint16_t width, height, sample_rate, channels;
 uint32_t video_remaining, audio_remaining, packet_end;
 /* Check packet boundary against the index without caching all keyframes.
  * These are the NEXT expected keyframe coordinates, updated by fseek
  * only when a keyframe is crossed (usually hundreds of frames apart). */
 uint32_t next_key_index, next_key_frame, next_key_offset;
 unsigned packet_open, current_is_keyframe;
} nds_fvds_stream;

typedef struct {
 uint32_t frame, video_bytes, audio_bytes, audio_blocks;
} nds_fvds_packet;

/* 0=success, 1=EOF, negative values are invalid data or I/O errors.
 * Stream owns only its FILE*; caller owns decoder, audio and display. */
int nds_fvds_open(nds_fvds_stream *s,const char *filename);
void nds_fvds_close(nds_fvds_stream *s);
int nds_fvds_next(nds_fvds_stream *s,nds_fvds_packet *pkt);
size_t nds_fvds_read_video(nds_fvds_stream *s,void *buf,size_t capacity);
size_t nds_fvds_read_audio(nds_fvds_stream *s,void *buf,size_t capacity);
int nds_fvds_finish(nds_fvds_stream *s);
int nds_fvds_seek_keyframe(nds_fvds_stream *s,uint32_t key_index);
#endif
