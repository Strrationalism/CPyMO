/* CPyMO 1.2 research-only FVDS on-device I/O preflight.
 * Independent container parser; intentionally NO video/audio decode,
 * VRAM setup, ARM7 changes, sound stream takeover or new gameplay commands.
 * Enabled only by an explicit nds-fvds-probe.flag inside the selected game.
 * A per-game text report enables physical NTR/TWL regression without debugger.
 */
#ifndef CPYMO_NDS_FVDS_PREFLIGHT_H
#define CPYMO_NDS_FVDS_PREFLIGHT_H

/* [0] magic, [1] runs, [2] pass, [3] fail, [4] frames scanned,
 * [5] video bytes, [6] audio bytes, [7] last code, [8] TWL, [9] I/O us */
/* [10] FNV1a payload checksum, [11] verified payload byte count */
volatile unsigned fvds_preflight_state[12]={0x46565031};

static unsigned fvds_le16(const unsigned char *p){
 return (unsigned)p[0] | ((unsigned)p[1]<<8);
}
static unsigned fvds_le32(const unsigned char *p){
 return fvds_le16(p) | (fvds_le16(p+2)<<16);
}
static bool fvds_read_exact(FILE *f,unsigned char *buf,unsigned n){
 return fread(buf,1,n,f)==n;
}
static bool fvds_skip_to(FILE *f,unsigned offset){
 return offset<=0x7fffffffu && fseek(f,(long)offset,SEEK_SET)==0;
}

/* Return true only when the explicit probe flag AND a .fv are present.
 * In all other cases the original .nvl playback path remains untouched. */
static bool nds_fvds_preflight(cpymo_engine *engine,cpymo_str name){
 FILE *opt=savefile(engine->assetloader.gamedir,"nds-fvds-probe.flag","rb");
 if(!opt)return false;
 fclose(opt);
 char path[512];
 nds_cache_path(path,sizeof path,engine->assetloader.gamedir,"video",name,".fv");
 FILE *f=fopen(path,"rb");
 if(!f)return false;

 fvds_preflight_state[1]++;
 fvds_preflight_state[8]=isDSiMode()?1:0;
 fvds_preflight_state[4]=fvds_preflight_state[5]=fvds_preflight_state[6]=0;
 unsigned result=1,frames=0,video_total=0,audio_total=0,keyframes=0;
 unsigned payload_digest=2166136261u,payload_read=0;
 /* Allocate this only for explicit I/O tests: ordinary NTR game memory
  * is not penalized with permanent, always-resident probe buffers. */
 unsigned char *io_buffer=malloc(4096);
 if(!io_buffer)result=17;
 const u64 started=tickGetCount();
 unsigned char header[28],key[8],packet[4];
 unsigned size=0,frame_count=0,data_start=0;
 unsigned next_key_frame=0,next_key_offset=0,next_key_idx=0,last_key_frame=0,last_key_offset=0;
 if(!io_buffer)goto close;
 const bool size_ok=fseek(f,0,SEEK_END)==0;
 long end=size_ok?ftell(f):-1;
 if(end<28 || end>192*1024*1024 || fseek(f,0,SEEK_SET)!=0){result=2;goto close;}
 size=(unsigned)end;
 if(!fvds_read_exact(f,header,sizeof header) || memcmp(header,"FVDS",4)){
  result=3;goto close;
 }
 const unsigned w=fvds_le16(header+4),h=fvds_le16(header+6);
 const unsigned fps_num=fvds_le32(header+8),fps_den=fvds_le32(header+12);
 const unsigned audio_rate=fvds_le16(header+16),channels=fvds_le16(header+18);
 frame_count=fvds_le32(header+20);keyframes=fvds_le32(header+24);
 if(w!=256||h!=192||!fps_num||fps_num>1000||!fps_den||fps_den>1000||
    audio_rate!=47605||channels!=2||!frame_count||frame_count>18000||
    !keyframes||keyframes>512||keyframes>frame_count){
  result=4;goto close;
 }
 data_start=28+keyframes*8;
 if(data_start>=size || (unsigned long long)data_start+(unsigned long long)frame_count*4>size){
  result=5;goto close;
 }
 /* Validate keyframe entries without allocation. A second file pointer is
  * unnecessary because frame packets start at the end of the key index. */
 for(unsigned i=0;i<keyframes;i++){
  if(!fvds_read_exact(f,key,sizeof key)){result=6;goto close;}
  unsigned ki=fvds_le32(key),ko=fvds_le32(key+4);
  if((i==0&&(ki!=0||ko!=data_start)) ||
     (i>0&&(ki<=last_key_frame||ko<=last_key_offset)) ||
     ki>=frame_count || ko>=size){
   result=7;goto close;
  }
  last_key_frame=ki;last_key_offset=ko;
 }
 /* Walk contiguous packets and compare each scheduled keyframe offset
  * to the exact boundary. The file cursor never leaves the game device. */
 if(!fvds_skip_to(f,28)){result=8;goto close;}
 if(!fvds_read_exact(f,key,sizeof key)){result=6;goto close;}
 next_key_frame=fvds_le32(key);
 next_key_offset=fvds_le32(key+4);
 next_key_idx=0;
 if(!fvds_skip_to(f,data_start)){result=8;goto close;}
 for(unsigned frame=0;frame<frame_count;frame++){
  long at=ftell(f);
  if(at<0 || (unsigned)at>=size){result=9;goto close;}
  if(frame==next_key_frame){
   if((unsigned)at!=next_key_offset){result=10;goto close;}
   next_key_idx++;
   if(next_key_idx<keyframes){
    long resume=ftell(f);
    if(!fvds_skip_to(f,28+next_key_idx*8) ||
       !fvds_read_exact(f,key,sizeof key) ||
       fseek(f,resume,SEEK_SET)){result=11;goto close;}
    next_key_frame=fvds_le32(key);
    next_key_offset=fvds_le32(key+4);
   }else next_key_frame=frame_count;
  }
  if(size-(unsigned)at<4 || !fvds_read_exact(f,packet,4)){result=12;goto close;}
  const unsigned value=fvds_le32(packet);
  const unsigned video_bytes=value&0x1ffffu,audio_blocks=value>>17;
  if(!video_bytes || (video_bytes&3u) || !audio_blocks || audio_blocks>255){
   result=13;goto close;
  }
  const unsigned audio_bytes=audio_blocks*2*132;
  long payload=ftell(f);
  if(payload<0 || (unsigned)payload>size ||
     (unsigned long long)payload+video_bytes+audio_bytes>size){
   result=14;goto close;
  }
  /* Unlike index-only scans, read the full video and ADPCM payload
   * through libfat. This proves sequential media transfer, not merely
   * successful fseek into FAT data clusters. No codec is invoked. */
  unsigned remaining=video_bytes+audio_bytes;
  while(remaining){
   unsigned n=remaining>4096?4096:remaining;
   if(!fvds_read_exact(f,io_buffer,n)){result=15;goto close;}
   for(unsigned i=0;i<n;i++)payload_digest=(payload_digest^io_buffer[i])*16777619u;
   payload_read+=n;remaining-=n;
   nds_audio_update();nds_power_poll();
  }
  frames++;video_total+=video_bytes;audio_total+=audio_bytes;
 }
 if(ftell(f)!=(long)size){result=16;goto close;}
 result=0;
close:
 fclose(f);free(io_buffer);
 fvds_preflight_state[4]=frames;
 fvds_preflight_state[5]=video_total;
 fvds_preflight_state[6]=audio_total;
 fvds_preflight_state[7]=result;
 fvds_preflight_state[9]=(unsigned)((tickGetCount()-started)*1000000ULL/TICK_FREQ);
 fvds_preflight_state[10]=payload_digest;
 fvds_preflight_state[11]=payload_read;
 if(result)fvds_preflight_state[3]++;else fvds_preflight_state[2]++;
 /* Opt-in file writes only; never touch game saves or ordinary game files.
  * This diagnostic intentionally leaves the original graphics and music active. */
 /* One report per movie avoids overwriting CHECK1 when CHECK3 is probed.
  * Sanitize untrusted script names before using them as filenames. */
 char report_name[80],safe_name[33]={0};
 unsigned j=0;
 for(unsigned i=0;i<name.len && j<32;i++){
  unsigned char c=(unsigned char)name.begin[i];
  safe_name[j++]=(c>='A'&&c<='Z')||(c>='a'&&c<='z')||
                 (c>='0'&&c<='9')||c=='_'||c=='-'?(char)c:'_';
 }
 if(!j)safe_name[0]='_',safe_name[1]=0;
 snprintf(report_name,sizeof report_name,"nds-fvds-probe-%s.txt",safe_name);
 FILE *log=savefile(engine->assetloader.gamedir,report_name,"wb");
 if(log){
  fprintf(log,"CPyMO 1.2 FVDS preflight (NO playback)\n");
  fprintf(log,"mode=%s\n",isDSiMode()?"DSi-TWL":"DS-NTR");
  fprintf(log,"result=%s code=%u\n",result?"FAILED":"OK",result);
  fprintf(log,"frames=%u/%u keys=%u video_bytes=%u audio_bytes=%u\n",
          frames,frame_count,keyframes,video_total,audio_total);
  fprintf(log,"file_bytes=%u io_microseconds=%u\n",size,fvds_preflight_state[9]);
  fprintf(log,"payload_read_bytes=%u payload_fnv1a32=%08X\n",
          payload_read,payload_digest);
  fprintf(log,"source=%s\n",path);
  fclose(log);
 }
 nds_audio_update();
 loaded_image=true;
 cpymo_engine_request_redraw(engine);
 return true;
}
#endif
