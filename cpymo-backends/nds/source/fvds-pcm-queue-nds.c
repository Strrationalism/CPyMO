#include "fvds-pcm-queue-nds.h"
#include <string.h>
#include <stdint.h>

int nds_fvds_pcm_queue_init(nds_fvds_pcm_queue *q,int16_t *memory,
                            size_t capacity){
 if(!q||!memory||capacity<256||capacity>16384 ||
    capacity>(SIZE_MAX/sizeof(int16_t))/2)return 0;
 memset(q,0,sizeof(*q));
 q->samples=memory;
 q->capacity=capacity;
 return 1;
}
void nds_fvds_pcm_queue_reset(nds_fvds_pcm_queue *q){
 if(!q)return;
 q->read_at=q->write_at=q->queued=q->high_watermark=0;
 q->produced=q->consumed=q->underflows=q->overflows=0;
}
int nds_fvds_pcm_queue_push(nds_fvds_pcm_queue *q,
                            const int16_t *interleaved,size_t frames){
 if(!q||!q->samples||(!interleaved&&frames))return 0;
 if(frames>q->capacity-q->queued){
  q->overflows++;
  return 0;
 }
 if(!frames)return 1;
 size_t first=frames<q->capacity-q->write_at?frames:q->capacity-q->write_at;
 memcpy(q->samples+2*q->write_at,interleaved,first*2*sizeof(int16_t));
 if(frames>first)
  memcpy(q->samples,interleaved+2*first,(frames-first)*2*sizeof(int16_t));
 q->write_at=(q->write_at+frames)%q->capacity;
 q->queued+=frames;
 q->produced+=frames;
 if(q->queued>q->high_watermark)q->high_watermark=q->queued;
 return 1;
}
size_t nds_fvds_pcm_queue_drain(nds_fvds_pcm_queue *q,
                                int16_t *dst,size_t frames){
 if(!q||!q->samples||(!dst&&frames))return 0;
 if(!frames)return 0;
 /* Large drains can be supported, but avoid size_t*4 overflow before
    calling memset on an uncontrolled output length. */
 if(frames>SIZE_MAX/(2*sizeof(int16_t)))return 0;
 size_t ready=frames<q->queued?frames:q->queued;
 size_t first=ready<q->capacity-q->read_at?ready:q->capacity-q->read_at;
 if(first)memcpy(dst,q->samples+2*q->read_at,first*2*sizeof(int16_t));
 if(ready>first)
  memcpy(dst+2*first,q->samples,(ready-first)*2*sizeof(int16_t));
 if(ready<frames){
  memset(dst+2*ready,0,(frames-ready)*2*sizeof(int16_t));
  q->underflows+=frames-ready;
 }
 q->read_at=(q->read_at+ready)%q->capacity;
 q->queued-=ready;
 q->consumed+=ready;
 return ready;
}
