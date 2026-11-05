/* Original FVDS elementary-picture adapter for the existing packet reader.
 * No frame cache or video decoder. Two readers MUST NOT consume one packet
 * concurrently: either stage callbacks OR this single elementary bit stream.
 */
#ifndef CPYMO_NDS_FVDS_PICTURE_SOURCE_H
#define CPYMO_NDS_FVDS_PICTURE_SOURCE_H
#include "fvds-stream-nds.h"
#include "fvds-bits-nds.h"

static inline size_t nds_fvds_picture_pull(void *ctx,void *data,size_t bytes){
 return nds_fvds_read_video((nds_fvds_stream*)ctx,data,bytes);
}
/* Must be called after nds_fvds_next() and before reading this packet's
 * first video byte; only the packet's VIDEO boundary is exposed. */
static inline int nds_fvds_picture_begin(nds_fvds_stream *movie,
                                         nds_fvds_bits *bits){
 if(!movie||!bits||!movie->packet_open ||
    !movie->video_remaining)return 0;
 return nds_fvds_bits_init(bits,nds_fvds_picture_pull,movie,
                           movie->video_remaining);
}
/* Once a picture decoder has consumed its payload, discard only the
 * remaining in-video padding/unused bytes so the audio blocks can follow.
 * Never read beyond packet.video_remaining into ADPCM. */
static inline int nds_fvds_picture_to_audio(nds_fvds_stream *movie,
                                            nds_fvds_bits *bits){
 if(!movie||!bits||!movie->packet_open||bits->failed)return 0;
 while(movie->video_remaining){
  size_t count=movie->video_remaining;
  if(count>sizeof(bits->bytes))count=sizeof(bits->bytes);
  if(nds_fvds_read_video(movie,bits->bytes,count)!=count)return 0;
 }
 bits->at=bits->available=bits->remaining_bits=0;
 return 1;
}
#endif
