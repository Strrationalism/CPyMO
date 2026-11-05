/* Exact largest 255-alpha run. Aligned word scan, scalar edges/mixed words.
 * A run is metadata only; failure to allocate it keeps the generic renderer. */
#include <stdint.h>
typedef uint32_t OpaqueWord __attribute__((__may_alias__));
static void opaque_span(const unsigned char* row,int left,int right,uint16_t* out){
 int x=left,start=left,best_l=left,best_r=left;
#define OPAQUE_STEP() do{if(row[x]!=255){if(x-start>best_r-best_l){best_l=start;best_r=x;}start=x+1;}x++;}while(0)
 while(x<right&&((uintptr_t)(row+x)&3))OPAQUE_STEP();
 while(x+4<=right){
  if(*(const OpaqueWord*)(row+x)==UINT32_MAX)x+=4;
  else{OPAQUE_STEP();OPAQUE_STEP();OPAQUE_STEP();OPAQUE_STEP();}
 }
 while(x<right)OPAQUE_STEP();
#undef OPAQUE_STEP
 if(right-start>best_r-best_l){best_l=start;best_r=right;}
 out[0]=best_l;out[1]=best_r;
}
