/* NPS1: row spans, exact RGB555/alpha reconstruction. No lossy quantization.
 * Bounded input buffer; omitted pixels are exactly RGB=0x8000, alpha=0.
 * Like NPI1 this raw container has length/bounds checks, not a content checksum. */
volatile unsigned sparse_state[8]={0x4e505331};
typedef struct {FILE* file;unsigned char* bytes;unsigned pos,size,left;unsigned char* encoded;bool compressed;} NpsInput;
static bool nps_copy(NpsInput* in,unsigned char* out,unsigned n){
 while(n){
  if(in->pos==in->size){
   unsigned want=in->left>8192?8192:in->left;if(!want)return false;
   u64 begin=tickGetCount();
   if(in->compressed){
    unsigned char h[8];if(fread(h,1,8,in->file)!=8)return false;
    unsigned packed=h[0]|(h[1]<<8),raw=h[2]|(h[3]<<8);uint32_t check=(uint32_t)h[4]|((uint32_t)h[5]<<8)|((uint32_t)h[6]<<16)|((uint32_t)h[7]<<24);
    if(raw!=want||!packed||packed>raw||(packed==raw&&check))return false;
    if(fread(packed==raw?in->bytes:in->encoded,1,packed,in->file)!=packed)return false;
    load_phase(4,begin);begin=tickGetCount();
    bool ok=true;if(packed<raw)ok=npl_decode(in->encoded,packed,in->bytes,raw);
    unsigned us=(tickGetCount()-begin)*1000000/TICK_FREQ;compressed_state[3]+=us;if(us>compressed_state[4])compressed_state[4]=us;
    if(!ok)return false;begin=tickGetCount();if(packed<raw&&npl_crc(in->bytes,raw)!=check)return false;
    compressed_state[5]+=(tickGetCount()-begin)*1000000/TICK_FREQ;compressed_state[1]+=raw;compressed_state[2]+=packed+8;compressed_state[6]++;
   }else if(fread(in->bytes,1,want,in->file)!=want)return false;
   in->size=want;in->pos=0;in->left-=want;load_phase(4,begin);
   begin=tickGetCount();nds_audio_update();load_phase(5,begin);
  }
  unsigned take=in->size-in->pos;if(take>n)take=n;
  memcpy(out,in->bytes+in->pos,take);in->pos+=take;out+=take;n-=take;
 }
 return true;
}
/* 1=success, 0=bad data, -1=allocation failure, for caller cache recovery. */
static int nps_file(FILE* file,Image* image,bool compressed){
 u64 started=tickGetCount();sparse_state[1]++;
 unsigned rows=image->ph,width=image->pw,count=rows*width;
 unsigned char* table=malloc(rows*4);unsigned char* buffer=malloc(8192);unsigned char* encoded=compressed?malloc(8192):NULL;
 if(!table||!buffer||(compressed&&!encoded)){free(table);free(buffer);free(encoded);sparse_state[6]++;return -1;}
 unsigned scratch=rows*4+8192+(compressed?8192:0);if(scratch>sparse_state[7])sparse_state[7]=scratch;
 bool ok=false;u64 begin=tickGetCount();unsigned stored=0;
 if(fread(table,1,rows*4,file)!=rows*4)goto done;
 for(unsigned y=0;y<rows;y++){
  const unsigned char* r=table+y*4;unsigned x=r[0]|(r[1]<<8),n=r[2]|(r[3]<<8);
  if(x>width||n>width-x)goto done;stored+=n;
 }
 if(!compressed&&!image_sequential&&(fseek(file,0,SEEK_END)||ftell(file)!=(long)(16+rows*4+stored*3)||fseek(file,16+rows*4,SEEK_SET)))goto done;
 load_phase(4,begin);
 for(unsigned i=0;i<count;i++)image->px[i]=0x8000;
 memset(image->mask,0,count);
 NpsInput in={file,buffer,0,0,stored*3,encoded,compressed};
 for(unsigned plane=0;plane<2;plane++)for(unsigned y=0;y<rows;y++){
  const unsigned char* r=table+y*4;unsigned x=r[0]|(r[1]<<8),n=r[2]|(r[3]<<8);
  unsigned char* out=plane?image->mask+y*width+x:(unsigned char*)(image->px+y*width+x);
  if(!nps_copy(&in,out,n*(plane?1:2)))goto done;
 }
 ok=in.left==0&&in.pos==in.size&&fgetc(file)==EOF&&!ferror(file);
done:free(table);free(buffer);free(encoded);
 if(ok){unsigned us=(tickGetCount()-started)*1000000/TICK_FREQ;sparse_state[2]+=count*3;sparse_state[3]+=16+rows*4+stored*3;sparse_state[4]+=us;if(us>sparse_state[5])sparse_state[5]=us;}else sparse_state[6]++;
 return ok;
}
