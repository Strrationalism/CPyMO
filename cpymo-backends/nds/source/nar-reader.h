/* Experimental independent bounded PCM16 predictor/Rice decoder. */
#include <stdint.h>
typedef struct {const unsigned char* p;unsigned size,pos,bits;uint32_t value;} NarBits;
static bool nar_get(NarBits* r,unsigned n,unsigned* out){
 while(r->bits<n){if(r->pos>=r->size)return false;r->value=(r->value<<8)|r->p[r->pos++];r->bits+=8;}
 r->bits-=n;*out=(r->value>>r->bits)&((1u<<n)-1);return true;
}
static bool nar_unary(NarBits* r,unsigned* out){
 unsigned q=0;
 for(;;){if(!r->bits){if(r->pos>=r->size)return false;r->value=r->p[r->pos++];r->bits=8;}
  uint32_t v=r->value<<(32-r->bits);unsigned z=v?__builtin_clz(v):r->bits;if(z>r->bits)z=r->bits;
  q+=z;if(q>31)return false;r->bits-=z;
  if(r->bits){r->bits--;*out=q;return true;}
 }
}
static uint32_t nar_checksum(const unsigned char* p,unsigned n){uint32_t a=1,b=0;while(n>=8){a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; n-=8;}while(n--){a+=*p++;b+=a;}return ((b%65521)<<16)|(a%65521);}
static bool nar_decode(const unsigned char* data,unsigned size,int16_t* dst,unsigned n,unsigned order,unsigned k){
 if(!n||n>1024||size>n*2||!size)return false;
 if(order==255){if(k||size!=n*2)return false;memcpy(dst,data,size);return true;}

 /* Private r10 block mode: 32/64/128-sample partitions with fixed-width
    zigzag residuals. No unary loop, no change to reconstructed PCM. */
 if(order>=16&&order<=18){
  unsigned predorder=order-16;
  if(k<5||k>7||n<=predorder||size<predorder*2)return false;
  for(unsigned i=0;i<predorder;i++)dst[i]=(int16_t)(data[2*i]|(data[2*i+1]<<8));
  NarBits r={data,size,predorder*2,0,0};
  for(unsigned start=predorder;start<n;){
   unsigned width;if(!nar_get(&r,5,&width)||width>16)return false;
   unsigned end=start+(1u<<k);if(end>n)end=n;
   for(;start<end;start++){
    unsigned z;if(!nar_get(&r,width,&z))return false;
    int d=(z>>1)^-(int)(z&1),pred=predorder==0?0:predorder==1?dst[start-1]:2*(int)dst[start-1]-dst[start-2];
    dst[start]=(int16_t)(pred+d);
   }
  }
  return r.pos==size&&r.bits<8&&!(r.value&((1u<<r.bits)-1));
 }

 if(order>=32&&order<=34){
  unsigned po=order-32;
  if(k<5||k>7||n<=po||size<po*2)return false;
  for(unsigned i=0;i<po;i++)dst[i]=(int16_t)(data[i*2]|(data[i*2+1]<<8));
  NarBits r={data,size,po*2,0,0};
  for(unsigned start=po;start<n;){
   unsigned key;if(!nar_get(&r,4,&key))return false;
   unsigned end=start+(1u<<k);if(end>n)end=n;
   for(;start<end;start++){
    unsigned q,lo;if(!nar_unary(&r,&q)||!nar_get(&r,key,&lo))return false;
    unsigned z=(q<<key)|lo;if(z>65535)return false;
    int d=(z>>1)^-(int)(z&1),pred=po==0?0:po==1?dst[start-1]:2*(int)dst[start-1]-dst[start-2];dst[start]=(int16_t)(pred+d);
   }
  }
  return r.pos==size&&r.bits<8&&!(r.value&((1u<<r.bits)-1));
 }
 if(order>2||k>15||n<=order||size<order*2)return false;
 for(unsigned i=0;i<order;i++)dst[i]=(int16_t)(data[i*2]|(data[i*2+1]<<8));
 NarBits r={data,size,order*2,0,0};
 for(unsigned i=order;i<n;i++){
  unsigned q,low;if(!nar_unary(&r,&q)||!nar_get(&r,k,&low))return false;unsigned z=(q<<k)|low;if(z>65535)return false;
  int d=(z>>1)^-(int)(z&1),pred=order==0?0:order==1?dst[i-1]:2*(int)dst[i-1]-dst[i-2];dst[i]=(int16_t)(pred+d);
 }
 return r.pos==size&&r.bits<8&&!(r.value&((1u<<r.bits)-1));
}
