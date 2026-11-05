/* Experimental bounded NPL4 reader, independent implementation. */
#include <stdint.h>
volatile unsigned compressed_state[8]={0x43504c34};
static uint32_t npl_crc(const unsigned char* p,unsigned n){
 uint32_t a=1,b=0;
 while(n){unsigned k=n>4096?4096:n;n-=k;
  while(k>=8){a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; a+=*p++;b+=a; k-=8;}
  while(k--){a+=*p++;b+=a;}a%=65521;b%=65521;
 }return (b<<16)|a;
}
static bool npl_length(const unsigned char* src,unsigned size,unsigned* pos,unsigned* n,unsigned limit){
 if(*n!=15)return true;
 for(;;){if(*pos>=size)return false;unsigned b=src[(*pos)++];*n+=b;if(*n>limit)return false;if(b!=255)return true;}
}
static bool npl_decode(const unsigned char* src,unsigned size,unsigned char* dst,unsigned expected){
 unsigned ip=0,op=0;
 while(ip<size){unsigned token=src[ip++],lit=token>>4;
  if(!npl_length(src,size,&ip,&lit,expected)||lit>size-ip||lit>expected-op)return false;
  memcpy(dst+op,src+ip,lit);ip+=lit;op+=lit;if(ip==size)return op==expected;
  if(size-ip<2)return false;unsigned offset=src[ip]|(src[ip+1]<<8);ip+=2;
  if(!offset||offset>op)return false;unsigned match=token&15;
  if(!npl_length(src,size,&ip,&match,expected)||match+4>expected-op)return false;match+=4;
  unsigned copied=match<offset?match:offset;memcpy(dst+op,dst+op-offset,copied);
  while(copied<match){unsigned n=match-copied;if(n>copied)n=copied;memcpy(dst+op+copied,dst+op,n);copied+=n;}op+=match;
 }return op==expected;
}
static bool npl_plane(FILE* file,unsigned char* dst,unsigned bytes,unsigned char* input){
 unsigned done=0;
 while(done<bytes){u64 start=tickGetCount();unsigned char h[8];if(fread(h,1,8,file)!=8)return false;
  unsigned stored=h[0]|(h[1]<<8),raw=h[2]|(h[3]<<8);uint32_t crc=(uint32_t)h[4]|((uint32_t)h[5]<<8)|((uint32_t)h[6]<<16)|((uint32_t)h[7]<<24);
  unsigned want=bytes-done;if(want>8192)want=8192;
  if(raw!=want||!stored||stored>raw||fread(input,1,stored,file)!=stored)return false;
  load_phase(4,start);start=tickGetCount();
  bool ok=true;if(stored==raw)memcpy(dst+done,input,raw);else ok=npl_decode(input,stored,dst+done,raw);
  unsigned decode_us=(tickGetCount()-start)*1000000/TICK_FREQ;compressed_state[3]+=decode_us;if(decode_us>compressed_state[4])compressed_state[4]=decode_us;
  if(!ok)return false;start=tickGetCount();if(npl_crc(dst+done,raw)!=crc)return false;
  compressed_state[5]+=(tickGetCount()-start)*1000000/TICK_FREQ;
  compressed_state[1]+=raw;compressed_state[2]+=stored+8;compressed_state[6]++;
  done+=raw;start=tickGetCount();nds_audio_update();load_phase(5,start);
 }return true;
}
