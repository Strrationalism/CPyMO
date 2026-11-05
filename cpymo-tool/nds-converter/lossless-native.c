/* MIT. Independent bounded PCM16/Rice block encoder, NDS port: 千岚之夏.
 * Host conversion only. This does not alter NDS decoding or audio quality. */
#include <stdint.h>
#include <string.h>
#define API __declspec(dllexport)
static int sample(const unsigned char* p,unsigned i){return (int16_t)(p[i*2]|((unsigned)p[i*2+1]<<8));}
API unsigned nar_encode_block(const unsigned char* raw,unsigned n,unsigned char* out,unsigned cap,unsigned* order_out,unsigned* k_out){
 if(!raw||!out||!n||n>1024||cap<n*2)return 0;
 unsigned residual[1024],chosen[1024],best=n*16,order=255,key=0;
 for(unsigned o=0;o<3&&o<n;o++){
  unsigned max=0;
  for(unsigned i=o;i<n;i++){
   int pred=o==0?0:o==1?sample(raw,i-1):2*sample(raw,i-1)-sample(raw,i-2);
   int d=(int16_t)(sample(raw,i)-pred);unsigned z=d<0?(unsigned)(-2*d-1):(unsigned)(2*d);
   residual[i]=z;if(z>max)max=z;
  }
  for(unsigned k=0;k<16;k++){
   if((max>>k)>31)continue;
   unsigned bits=o*16+(n-o)*(k+1);if(bits>=best)continue;
   for(unsigned i=o;i<n;i++)bits+=residual[i]>>k;
   if(bits<best){best=bits;order=o;key=k;memcpy(chosen,residual,n*sizeof(unsigned));}
  }
 }
 *order_out=order;*k_out=key;
 if(order==255){memcpy(out,raw,n*2);return n*2;}
 memcpy(out,raw,order*2);unsigned pos=order*2,used=0;uint64_t acc=0;
 for(unsigned i=order;i<n;i++){
  unsigned q=chosen[i]>>key;acc=(acc<<(q+1))|1;used+=q+1;
  while(used>=8){used-=8;out[pos++]=(unsigned char)(acc>>used);}acc&=((UINT64_C(1)<<used)-1);
  acc=(acc<<key)|(chosen[i]&((1u<<key)-1));used+=key;
  while(used>=8){used-=8;out[pos++]=(unsigned char)(acc>>used);}acc&=((UINT64_C(1)<<used)-1);
 }
 if(used)out[pos++]=(unsigned char)(acc<<(8-used));return pos;
}
/* Independent simple bit-reader round trip for conversion integrity checking. */
static int get(const unsigned char* p,unsigned bytes,unsigned* bit,unsigned n,unsigned* out){
 if(*bit+n>bytes*8)return 0;unsigned v=0;while(n--){v=(v<<1)|((p[*bit/8]>>(7-*bit%8))&1);(*bit)++;}*out=v;return 1;
}
API unsigned nar_validate_block(const unsigned char* data,unsigned bytes,const unsigned char* original,unsigned n,unsigned order,unsigned k){
 if(!n||n>1024||!bytes||bytes>n*2||k>15)return 0;
 if(order==255)return !k&&bytes==n*2&&!memcmp(data,original,bytes);
 if(order>2||n<=order||bytes<order*2||memcmp(data,original,order*2))return 0;
 int prev=order?sample(original,order-1):0,prev2=order>1?sample(original,order-2):0;unsigned bit=order*16;
 for(unsigned i=order;i<n;i++){
  unsigned q=0,b,lo;do{if(!get(data,bytes,&bit,1,&b))return 0;if(!b&&++q>31)return 0;}while(!b);
  if(!get(data,bytes,&bit,k,&lo))return 0;unsigned z=(q<<k)|lo;if(z>65535)return 0;
  int d=(int)(z>>1)^-(int)(z&1),pred=order==0?0:order==1?prev:2*prev-prev2;int v=(int16_t)(pred+d);
  if(v!=sample(original,i))return 0;prev2=prev;prev=v;
 }
 return (bit+7)/8==bytes&&(!(bit%8)||!(data[bytes-1]&((1u<<(8-bit%8))-1)));
}

/* MIT. Host experimental encoder; private block orders 16..18. */
#include <stdint.h>
#include <string.h>
static unsigned zz(int d){d=(int16_t)d;return ((unsigned)d<<1)^((unsigned)(d>>31));}
static unsigned width(unsigned x){return x?32-__builtin_clz(x):0;}
typedef struct{unsigned char *p;unsigned pos,bits;uint32_t v;} Bits;
static void put(Bits*b,unsigned n,unsigned v){b->v=(b->v<<n)|v;b->bits+=n;while(b->bits>=8){b->bits-=8;b->p[b->pos++]=(unsigned char)(b->v>>b->bits);}}
__declspec(dllexport) unsigned pack_encode(const unsigned char* raw,unsigned n,unsigned char*out,unsigned part,unsigned*order){
 if(!n||n>1024||part<5||part>7)return 0;
 int16_t s[1024];for(unsigned i=0;i<n;i++)s[i]=(int16_t)(raw[i*2]|raw[i*2+1]<<8);
 unsigned best=n*16,bo=255;
 for(unsigned o=0;o<=2&&o<n;o++){
  unsigned cost=o*16;
  for(unsigned i=o;i<n;){unsigned mx=0,end=i+(1u<<part);if(end>n)end=n;unsigned start=i;
   for(;i<end;i++){int pred=o==0?0:o==1?s[i-1]:2*(int)s[i-1]-s[i-2];unsigned z=zz((int)s[i]-pred);if(z>mx)mx=z;}
   cost+=5+(end-start)*width(mx);
  }
  if(cost<best){best=cost;bo=o;}
 }
 if(bo==255||((best+7)/8)>=n*2){memcpy(out,raw,n*2);*order=255;return n*2;}
 memcpy(out,raw,bo*2);Bits b={out,bo*2,0,0};
 for(unsigned i=bo;i<n;){unsigned mx=0,end=i+(1u<<part);if(end>n)end=n;
  for(unsigned j=i;j<end;j++){int pred=bo==0?0:bo==1?s[j-1]:2*(int)s[j-1]-s[j-2];unsigned z=zz((int)s[j]-pred);if(z>mx)mx=z;}
  unsigned w=width(mx);put(&b,5,w);
  for(;i<end;i++){int pred=bo==0?0:bo==1?s[i-1]:2*(int)s[i-1]-s[i-2];put(&b,w,zz((int)s[i]-pred));}
 }
 if(b.bits)out[b.pos++]=(unsigned char)(b.v<<(8-b.bits));*order=16+bo;return b.pos;
}
/* Rice parameters selected separately for each bounded partition. */
__declspec(dllexport) unsigned part_encode(const unsigned char*raw,unsigned n,unsigned char*out,unsigned part,unsigned*order){
 if(!n||n>1024||part<5||part>7)return 0;
 int16_t s[1024];unsigned z[1024],keys[32],bestkeys[32];for(unsigned i=0;i<n;i++)s[i]=(int16_t)(raw[i*2]|raw[i*2+1]<<8);
 unsigned best=n*16,bo=255;
 for(unsigned o=0;o<3&&o<n;o++){
  for(unsigned i=o;i<n;i++){int pred=o==0?0:o==1?s[i-1]:2*(int)s[i-1]-s[i-2];z[i]=zz((int)s[i]-pred);}
  unsigned cost=o*16,group=0;
  for(unsigned start=o;start<n;start+=(1u<<part)){
   unsigned end=start+(1u<<part);if(end>n)end=n;unsigned mx=0;
   for(unsigned i=start;i<end;i++)if(z[i]>mx)mx=z[i];
   unsigned bc=~0u,bk=0;
   for(unsigned k=0;k<16;k++){
    if((mx>>k)>31)continue;
    unsigned bits=(end-start)*(k+1);for(unsigned i=start;i<end;i++)bits+=z[i]>>k;
    if(bits<bc){bc=bits;bk=k;}
   }
   keys[group++]=bk;cost+=4+bc;
  }
  if(cost<best){best=cost;bo=o;memcpy(bestkeys,keys,group*sizeof(unsigned));}
 }
 if(bo==255||((best+7)/8)>=n*2){memcpy(out,raw,n*2);*order=255;return n*2;}
 memcpy(out,raw,bo*2);Bits b={out,bo*2,0,0};unsigned group=0;
 for(unsigned start=bo;start<n;start+=(1u<<part)){
  unsigned end=start+(1u<<part);if(end>n)end=n;unsigned k=bestkeys[group++];put(&b,4,k);
  for(unsigned i=start;i<end;i++){
   int pred=bo==0?0:bo==1?s[i-1]:2*(int)s[i-1]-s[i-2];unsigned v=zz((int)s[i]-pred),q=v>>k;
   /* Emit long unary words in bounded pieces so the 32-bit reservoir cannot overflow. */
   while(q>=16){put(&b,16,0);q-=16;}put(&b,q+1,1);put(&b,k,v&((1u<<k)-1));
  }
 }
 if(b.bits)out[b.pos++]=(unsigned char)(b.v<<(8-b.bits));*order=32+bo;return b.pos;
}

/* Independent simple bit-reader validation for new NAR2 modes. */
API unsigned nar2_validate_block(const unsigned char* data,unsigned bytes,const unsigned char* original,unsigned n,unsigned order,unsigned k){
 if(order<=2||order==255)return nar_validate_block(data,bytes,original,n,order,k);
 if(!n||n>1024||!bytes||bytes>n*2||k<5||k>7)return 0;
 unsigned po,partition=0;
 if(order>=16&&order<=18)po=order-16;
 else if(order>=32&&order<=34){po=order-32;partition=1;}else return 0;
 if(n<=po||bytes<po*2||memcmp(data,original,po*2))return 0;
 unsigned bit=po*16;
 for(unsigned start=po;start<n;){
  unsigned width;if(!get(data,bytes,&bit,partition?4:5,&width)||width>16)return 0;
  unsigned end=start+(1u<<k);if(end>n)end=n;
  for(;start<end;start++){
   unsigned z;
   if(partition){unsigned q=0,b,lo;do{if(!get(data,bytes,&bit,1,&b))return 0;if(!b&&++q>31)return 0;}while(!b);if(!get(data,bytes,&bit,width,&lo))return 0;z=(q<<width)|lo;if(z>65535)return 0;}
   else if(!get(data,bytes,&bit,width,&z))return 0;
   int d=(z>>1)^-(int)(z&1),pred=po==0?0:po==1?sample(original,start-1):2*sample(original,start-1)-sample(original,start-2);
   if((int16_t)(pred+d)!=sample(original,start))return 0;
  }
 }
 return (bit+7)/8==bytes&&(!(bit%8)||!(data[bytes-1]&((1u<<(8-bit%8))-1)));
}
