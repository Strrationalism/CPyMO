#include "fvds-bits-nds.h"
#include <string.h>

int nds_fvds_bits_init(nds_fvds_bits *b,nds_fvds_bit_pull pull,
                       void *context,uint32_t picture_bytes){
 if(!b)return 0;
 memset(b,0,sizeof *b);
 if(!pull || picture_bytes<2 || picture_bytes>131068u ||
    (picture_bytes&1u)){b->failed=1;return 0;}
 b->pull=pull;
 b->context=context;
 b->total_bytes=picture_bytes;
 return 1;
}
static int next_word(nds_fvds_bits *b){
 if(b->at==b->available){
  if(b->fetched_bytes>=b->total_bytes)return 0;
  size_t count=b->total_bytes-b->fetched_bytes;
  if(count>sizeof b->bytes)count=sizeof b->bytes;
  /* All storage words are two bytes; chunks always end on word boundaries. */
  if(b->pull(b->context,b->bytes,count)!=count)return 0;
  b->fetched_bytes+=(uint32_t)count;
  b->available=count;
  b->at=0;
 }
 if(b->available-b->at<2)return 0;
 b->word=(uint16_t)((unsigned)b->bytes[b->at]|
                    ((unsigned)b->bytes[b->at+1]<<8));
 b->at+=2;
 b->remaining_bits=16;
 return 1;
}
int nds_fvds_bits_read(nds_fvds_bits *b,unsigned count,uint32_t *value){
 if(!b||!value||b->failed||!count||count>16)return 0;
 uint32_t result=0;
 unsigned needed=count;
 while(needed){
  if(!b->remaining_bits&&!next_word(b)){
   b->failed=1;
   return 0;
  }
  unsigned take=needed<b->remaining_bits?needed:b->remaining_bits;
  unsigned shift=b->remaining_bits-take;
  unsigned mask=(1u<<take)-1u;
  result=(result<<take)|((b->word>>shift)&mask);
  b->remaining_bits-=take;
  b->consumed_bits+=take;
  needed-=take;
 }
 *value=result;
 return 1;
}
int nds_fvds_bits_align16(nds_fvds_bits *b){
 if(!b||b->failed)return 0;
 b->consumed_bits+=b->remaining_bits;
 b->remaining_bits=0;
 return 1;
}
