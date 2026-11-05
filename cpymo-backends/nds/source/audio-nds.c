#include <nds.h>
#include <maxmod9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpymo_engine.h"
#include "cpymo_audio.h"
#include "config.h"
#include "global.h"
#include "fixed.h"
#include "bit.h"
#include "timer.h"
#include "stream.h"
#include "frame.h"
#include "synth.h"
#include "cache-nds.h"
#include <errno.h>
#include "audio-gain-nds.h"
extern volatile unsigned engine_state[46];
extern volatile unsigned vblank_count;
typedef struct {struct mad_stream stream;struct mad_frame frame;struct mad_synth synth;unsigned char input[8192+MAD_BUFFER_GUARD];unsigned used;bool padded;} MP3;
#include "nar-reader.h"
typedef struct {unsigned total,left;bool modern;unsigned char input[2048];} NAR;
volatile unsigned audio_state[12]={0x4e415235};
volatile unsigned audio_bench[12]={0x4e415235};
typedef struct {NAR* nar;FILE* file;bool loop;int volume;char name[64];s16 cache[1152];unsigned pos,count;unsigned offset,length,remaining,rate,phase;MP3* mp3;} Channel;
typedef struct {Channel ch[3];int nar_policy;unsigned char* aliases;unsigned alias_count;bool alias_checked;error_t alias_error;} Audio;
static Audio* active;
static NdsAudioGain nds_gain;
#include "audio-aliases.h"
/* Optional delivered-PCM verification, disabled in normal/performance runs. */
volatile unsigned movie_audio_check[8]={0x4d414331};static bool movie_verify;
void nds_movie_audio_verify(bool enabled){movie_verify=enabled;for(unsigned i=1;i<8;i++)movie_audio_check[i]=0;movie_audio_check[2]=1;}
extern volatile unsigned movie_state[24];static FILE* movie_audio;static unsigned movie_remaining;static Channel* movie_channel;

/* In-memory diagnostic ring, no filesystem writes from the audio service. */
volatile unsigned audio_trace[4+128*8]={0x47545237};
static unsigned last_service_us,last_service_pc;
static u64 last_audio_tick;
static unsigned last_audio_vblank;
static bool audio_suspended;
extern volatile unsigned sleep_state[12];
extern void nds_mm_stream_suspend(void);
extern void nds_mm_stream_resume(void);
static void stop(Channel* c){free(c->nar);c->nar=NULL;if(c->file)fclose(c->file);if(c->mp3){mad_synth_finish(&c->mp3->synth);mad_frame_finish(&c->mp3->frame);mad_stream_finish(&c->mp3->stream);free(c->mp3);}c->mp3=NULL;c->file=NULL;c->pos=c->count=0;c->name[0]=0;}
static bool refill_mp3(Channel* c){
 MP3* d=c->mp3;unsigned keep=0;
 if(d->stream.next_frame&&d->stream.next_frame>=d->input&&d->stream.next_frame<=d->input+d->used){keep=d->input+d->used-d->stream.next_frame;memmove(d->input,d->stream.next_frame,keep);}
 unsigned need=8192-keep;if(need>c->remaining)need=c->remaining;
 unsigned n=fread(d->input+keep,1,need,c->file);c->remaining-=n;
 if(n!=need){engine_state[19]++;return false;}
 d->used=keep+n;
 if(!c->remaining){if(d->padded)return false;memset(d->input+d->used,0,MAD_BUFFER_GUARD);d->used+=MAD_BUFFER_GUARD;d->padded=true;}
 mad_stream_buffer(&d->stream,d->input,d->used);return d->used>0;
}
static s16 pcm16(mad_fixed_t s){s+=1L<<(MAD_F_FRACBITS-16);if(s>=MAD_F_ONE)s=MAD_F_ONE-1;if(s< -MAD_F_ONE)s=-MAD_F_ONE;return s>>(MAD_F_FRACBITS+1-16);}
static bool decode_mp3(Channel* c){
 MP3* d=c->mp3;
 unsigned started=vblank_count;
 u64 started_tick=tickGetCount();
 for(unsigned tries=0;tries<4096;tries++){
  if(!d->stream.buffer||d->stream.error==MAD_ERROR_BUFLEN){if(!refill_mp3(c)){
   if(!c->loop)return false;
   if(fseek(c->file,c->offset,SEEK_SET))return false;
   c->remaining=c->length;mad_stream_finish(&d->stream);mad_stream_init(&d->stream);d->used=0;d->padded=false;mad_frame_mute(&d->frame);mad_synth_mute(&d->synth);continue;
  }}
  if(mad_frame_decode(&d->frame,&d->stream)==-1){if(d->stream.error==MAD_ERROR_BUFLEN||MAD_RECOVERABLE(d->stream.error))continue;engine_state[19]++;return false;}
  // Use libmad's half-rate synthesis for high-rate sources; no offline conversion.
  if(d->frame.header.samplerate>=32000)d->frame.options|=MAD_OPTION_HALFSAMPLERATE;
  mad_synth_frame(&d->synth,&d->frame);c->count=d->synth.pcm.length;c->pos=0;c->rate=d->synth.pcm.samplerate;
  for(unsigned i=0;i<c->count;i++){mad_fixed_t s=d->synth.pcm.samples[0][i];if(d->synth.pcm.channels==2)s=(s/2)+(d->synth.pcm.samples[1][i]/2);c->cache[i]=pcm16(s);}
  engine_state[21]++;unsigned took=vblank_count-started;if(took>engine_state[24])engine_state[24]=took;
  unsigned us=(tickGetCount()-started_tick)*1000000/TICK_FREQ;if(us>engine_state[26])engine_state[26]=us;
  return c->count>0;
 }
 engine_state[19]++;return false;
}
static unsigned refill_pcm(Channel* c){
 if(!c->nar)return fread(c->cache,sizeof(s16),1024,c->file);
 NAR* d=c->nar;
 if(!d->left){if(!c->loop||!d->total)return 0;if(fseek(c->file,16,SEEK_SET))goto bad;d->left=d->total;audio_state[9]++;}
 unsigned char h[12];u64 start=tickGetCount();if(fread(h,1,12,c->file)!=12)goto bad;
 unsigned size=h[0]|(h[1]<<8),n=h[2]|(h[3]<<8),order=h[4],k=h[5];uint32_t check=h[8]|((uint32_t)h[9]<<8)|((uint32_t)h[10]<<16)|((uint32_t)h[11]<<24);
 if(n!=(d->left>1024?1024:d->left)||!size||size>n*2||h[6]||h[7]||fread(d->input,1,size,c->file)!=size)goto bad;
 audio_state[2]+=(tickGetCount()-start)*1000000/TICK_FREQ;start=tickGetCount();
 if((!d->modern&&order>=16&&order!=255)||!nar_decode(d->input,size,c->cache,n,order,k)||nar_checksum((unsigned char*)c->cache,n*2)!=check)goto bad;
 unsigned us=(tickGetCount()-start)*1000000/TICK_FREQ;audio_state[1]++;audio_state[3]+=us;if(us>audio_state[4])audio_state[4]=us;audio_state[5]+=size+12;audio_state[6]+=n*2;
 d->left-=n;if(!d->left&&!c->loop)audio_state[10]++;if(!d->left&&(fgetc(c->file)!=EOF||ferror(c->file)))goto bad;return n;
bad:audio_state[7]++;engine_state[19]++;return 0;
}
static int next(Channel* c){
 if(!c->file)return 0;
 if(c->mp3){
  if(c->pos>=c->count&&!decode_mp3(c)){stop(c);return 0;}
  int sample=c->cache[c->pos]*c->volume/256;c->phase+=c->rate;
  while(c->phase>=22050){c->phase-=22050;++c->pos;if(c->pos>=c->count&&!decode_mp3(c)){stop(c);break;}}
  return sample;
 }
 if(c->pos==c->count){
  c->count=refill_pcm(c);c->pos=0;
  if(!c->count&&c->loop&&!c->nar){rewind(c->file);c->count=fread(c->cache,sizeof(s16),1024,c->file);}
  if(!c->count){if(ferror(c->file))engine_state[19]++;stop(c);return 0;}
 }
 return c->cache[c->pos++]*c->volume/256;
}
static mm_word refill(mm_word length,mm_addr dest,mm_stream_formats format){
 (void)format;s16* out=dest;engine_state[17]++;
 if(movie_audio){unsigned want=length;if(want>movie_remaining)want=movie_remaining;unsigned got=0;
  if(movie_channel){while(got<want){Channel* c=movie_channel;if(c->pos==c->count){c->count=refill_pcm(c);c->pos=0;if(!c->count)break;}unsigned n=c->count-c->pos;if(n>want-got)n=want-got;memcpy(out+got,c->cache+c->pos,n*2);c->pos+=n;got+=n;}}
  else got=fread(out,2,want,movie_audio);if(movie_verify&&got){unsigned c=nar_checksum((unsigned char*)out,got*2),a=movie_audio_check[2],b=movie_audio_check[3];movie_audio_check[1]+=got;movie_audio_check[2]=(a+(c&65535)-1)%65521;movie_audio_check[3]=(b+(c>>16)+got*2*(a-1))%65521;}
 if(got!=want){movie_state[7]++;movie_audio_check[4]++;movie_remaining=0;}else movie_remaining-=got;
  for(unsigned i=0;i<got;i++)if(out[i]){engine_state[18]++;movie_state[14]++;}
  /* Checksum above tests original decoded PCM. Apply gain to playback after
     the check, including movie PCM and NAR2, not the verification bytes. */
  for(unsigned done=0;done<got;){
   unsigned batch=got-done;if(batch>64)batch=64;
   int32_t raw[64];for(unsigned i=0;i<batch;i++)raw[i]=out[done+i];
   nds_audio_gain_write(out+done,raw,batch,&nds_gain);
   done+=batch;
  }
  if(got<length)memset(out+got,0,(length-got)*2);
  movie_state[13]++;return length;}

 bool pcm_only=active!=NULL;
 if(active)for(unsigned c=0;c<3;c++)if(active->ch[c].mp3)pcm_only=false;
 if(!pcm_only){
  for(unsigned done=0;done<length;){
    unsigned batch=length-done;if(batch>64)batch=64;
    int32_t mixed[64];
    for(unsigned i=0;i<batch;i++){
     int sample=0;if(active)for(unsigned c=0;c<3;++c)sample+=next(&active->ch[c]);
     mixed[i]=sample;
    }
    nds_audio_gain_write(out+done,mixed,batch,&nds_gain);
    for(unsigned i=0;i<batch;i++)if(out[done+i])engine_state[18]++;
    done+=batch;
   }
  return length;
 }
 for(unsigned done=0;done<length;){
  unsigned batch=length-done;if(batch>64)batch=64;int32_t mixed[64]={0};
  for(unsigned channel=0;channel<3;channel++){
   Channel* c=&active->ch[channel];unsigned pos=0;
   while(pos<batch&&c->file){
    if(c->pos==c->count){c->count=refill_pcm(c);c->pos=0;
     if(!c->count&&c->loop&&!c->nar){rewind(c->file);c->count=fread(c->cache,sizeof(s16),1024,c->file);}
     if(!c->count){if(ferror(c->file))engine_state[19]++;stop(c);break;}
    }
    unsigned count=c->count-c->pos;if(count>batch-pos)count=batch-pos;
    for(unsigned i=0;i<count;i++)mixed[pos+i]+=c->cache[c->pos+i]*c->volume/256;
    c->pos+=count;pos+=count;
   }
  }
  nds_audio_gain_write(out+done,mixed,batch,&nds_gain);
   for(unsigned i=0;i<batch;i++)if(out[done+i])engine_state[18]++;
  done+=batch;
 }return length;
}

static void movie_stream(unsigned size){mm_stream stream={0};stream.sampling_rate=22050;stream.buffer_length=size;stream.callback=refill;stream.format=MM_STREAM_16BIT_MONO;stream.timer=MM_TIMER0;stream.manual=true;mmStreamOpen(&stream);last_audio_tick=tickGetCount();last_audio_vblank=vblank_count;}
static bool movie_audio_begin(const char* path,unsigned offset,unsigned samples,bool compressed){
 FILE* f=fopen(path,"rb");if(!f)return false;
 if(compressed&&setvbuf(f,NULL,_IOFBF,2048)){fclose(f);return false;}
 if(fseek(f,offset,SEEK_SET)){fclose(f);return false;}
 Channel* c=NULL;
 if(compressed){
  unsigned char h[16];if(fread(h,1,16,f)!=16||memcmp(h,"NAR2",4)||h[8]!=0x22||h[9]!=0x56||h[10]||h[11]||h[12]||h[13]||h[14]||h[15]||(h[4]|((unsigned)h[5]<<8)|((unsigned)h[6]<<16)|((unsigned)h[7]<<24))!=samples){fclose(f);return false;}
  c=calloc(1,sizeof *c);if(!c){fclose(f);return false;}c->nar=calloc(1,sizeof *c->nar);if(!c->nar){free(c);fclose(f);return false;}
  c->file=f;c->nar->total=c->nar->left=samples;c->nar->modern=true;
 }
 movie_audio_check[5]=compressed;mmStreamClose();movie_channel=c;movie_audio=f;movie_remaining=samples;nds_audio_gain_reset(&nds_gain);movie_stream(4096);return true;
}
bool nds_movie_audio_begin(const char* path,unsigned offset,unsigned samples){return movie_audio_begin(path,offset,samples,false);}
bool nds_movie_audio_begin_nar(const char* path,unsigned offset,unsigned samples){return movie_audio_begin(path,offset,samples,true);}
unsigned nds_movie_audio_position(void){return mmStreamGetPosition();}
void nds_movie_audio_end(void){if(!movie_audio)return;mmStreamClose();if(movie_channel){stop(movie_channel);free(movie_channel);movie_channel=NULL;}else fclose(movie_audio);movie_audio=NULL;nds_audio_gain_reset(&nds_gain);movie_stream(nds_cache_enabled?4096:2048);}
void cpymo_audio_init(cpymo_audio_system* s){
 Audio* a=calloc(1,sizeof *a);*s=a;active=a;if(a)a->nar_policy=-2;last_audio_tick=tickGetCount();nds_audio_gain_reset(&nds_gain);if(!a)return;
 for(unsigned c=0;c<3;++c)a->ch[c].volume=256;
 mm_ds_system sys={0};mmInit(&sys);
 mm_stream stream={0};stream.sampling_rate=22050;stream.buffer_length=nds_cache_enabled?4096:2048;stream.callback=refill;stream.format=MM_STREAM_16BIT_MONO;stream.timer=MM_TIMER0;stream.manual=true;
 mmStreamOpen(&stream);engine_state[15]=1;
}
void nds_audio_update(void){if(active&&!audio_suspended){u64 started_tick=tickGetCount();bool playing=active->ch[0].file||active->ch[1].file||active->ch[2].file;if(playing&&last_audio_tick){unsigned gap=(started_tick-last_audio_tick)*1000000/TICK_FREQ;if(gap>engine_state[28])engine_state[28]=gap;if(gap>=30000){unsigned seq=audio_trace[1]++,i=4+(seq%128)*8;audio_trace[i]=seq;audio_trace[i+1]=gap;audio_trace[i+2]=last_service_us;audio_trace[i+3]=last_service_pc;audio_trace[i+4]=(unsigned)__builtin_return_address(0);audio_trace[i+5]=engine_state[14];audio_trace[i+6]=vblank_count;audio_trace[i+7]=audio_state[1];if(gap>audio_trace[2])audio_trace[2]=gap;}}last_audio_tick=started_tick;unsigned started=vblank_count;mmStreamUpdate();unsigned took=vblank_count-started;if(took>engine_state[25])engine_state[25]=took;unsigned us=(tickGetCount()-started_tick)*1000000/TICK_FREQ;if(us>engine_state[27])engine_state[27]=us;last_service_us=us;last_service_pc=(unsigned)__builtin_return_address(0);last_audio_vblank=vblank_count;}}
void nds_audio_suspend(void){
 if(!active||audio_suspended)return;
 nds_audio_update();sleep_state[7]=mmStreamGetPosition();
 nds_mm_stream_suspend();soundSetMixerVolume(0);soundSynchronize();audio_suspended=true;
 sleep_state[5]++;sleep_state[8]=0;for(unsigned c=0;c<3;c++)if(active->ch[c].file)sleep_state[8]++;
}
void nds_audio_resume(void){
 if(!active||!audio_suspended)return;
 sleep_state[9]=210;nds_mm_stream_resume();sleep_state[9]=220;soundSetMixerVolume(127);sleep_state[9]=230;soundSynchronize();sleep_state[9]=240;audio_suspended=false;
 last_audio_tick=tickGetCount();last_audio_vblank=vblank_count;sleep_state[6]++;sleep_state[9]=0;
}
/* A cheap display-tick guard: no clock read or mixer call for every row. */
void nds_audio_pump_if_due(void){if(active&&vblank_count!=last_audio_vblank){nds_audio_update();last_audio_vblank=vblank_count;}}
void cpymo_audio_free(cpymo_audio_system* s){Audio* a=*s;if(a){mmStreamClose();for(unsigned c=0;c<3;++c)stop(&a->ch[c]);free(a->aliases);free(a);}if(active==a)active=NULL;*s=NULL;}
bool cpymo_audio_enabled(cpymo_engine* e){return e->audio!=NULL;}
float cpymo_audio_get_channel_volume(size_t c,const cpymo_audio_system* s){return *s?((Audio*)*s)->ch[c].volume/256.f:0;}
void cpymo_audio_set_channel_volume(size_t c,cpymo_audio_system* s,float v){if(v<0)v=0;if(v>1)v=1;if(*s)((Audio*)*s)->ch[c].volume=(int)(v*256);}
bool cpymo_audio_channel_is_playing(size_t c,cpymo_audio_system* s){return *s&&((Audio*)*s)->ch[c].file;}
bool cpymo_audio_channel_is_looping(size_t c,cpymo_audio_system* s){return *s&&((Audio*)*s)->ch[c].loop;}
bool cpymo_audio_wait_se(cpymo_engine* e,float d){(void)d;return !cpymo_audio_channel_is_playing(1,&e->audio);}
static bool known_missing_audio(const char* dir,const char* kind,cpymo_str name){
 char expected[512],path[512],line[96],entry[48];nds_cache_path(expected,sizeof expected,"",kind,name,"");
 const char* hash=strrchr(expected,'/');if(!hash)return false;
 snprintf(entry,sizeof entry,"%s %s\n",kind,hash+1);snprintf(path,sizeof path,"%s/nds-cache/missing-audio.txt",dir);
 FILE* file=fopen(path,"rb");if(!file)return false;bool found=false;
 while(fgets(line,sizeof line,file))if(!strcmp(line,entry)){found=true;break;}
 fclose(file);return found;
}
static error_t play(cpymo_engine* e,cpymo_str name,unsigned id,bool loop){
 Audio* a=e->audio;if(!a)return CPYMO_ERR_OUT_OF_MEM;
 Channel* c=&a->ch[id];stop(c);char path[512];const char* folder=id==0?"bgm":id==1?"se":"voice";
 if(name.len>=sizeof c->name)return CPYMO_ERR_INVALID_ARG;
 memcpy(c->name,name.begin,name.len);c->name[name.len]=0;
 if(nds_cache_enabled){
  error_t alias_error=load_aliases(a,e->assetloader.gamedir);if(alias_error)return alias_error;
  if(a->nar_policy==-2){a->nar_policy=-1;snprintf(path,sizeof path,"%s/nds-cache/audio-formats.bin",e->assetloader.gamedir);FILE* f=fopen(path,"rb");if(f){unsigned char h[6];unsigned got=fread(h,1,6,f);fclose(f);if(got!=5||memcmp(h,"NAF1",4)||(h[4]&~7)){engine_state[19]++;/* Optional index is invalid: use checked legacy detection. */}else a->nar_policy=h[4];}}
  bool try_nar=a->nar_policy<0||(a->nar_policy&(1<<id));
  if(try_nar){audio_cache_path(path,sizeof path,e->assetloader.gamedir,folder,name,".nar",a,id);c->file=fopen(path,"rb");}else errno=ENOENT;
  if(c->file){if(setvbuf(c->file,NULL,_IOFBF,2048)){stop(c);return CPYMO_ERR_OUT_OF_MEM;}unsigned char h[16];if(fread(h,1,16,c->file)!=16||(memcmp(h,"NAR1",4)&&memcmp(h,"NAR2",4))||h[8]!=0x22||h[9]!=0x56||h[10]||h[11]||h[12]||h[13]||h[14]||h[15]){stop(c);engine_state[19]++;return CPYMO_ERR_BAD_FILE_FORMAT;}
   c->nar=calloc(1,sizeof *c->nar);if(!c->nar){stop(c);return CPYMO_ERR_OUT_OF_MEM;}
   c->nar->modern=h[3]=='2';c->nar->total=c->nar->left=h[4]|((unsigned)h[5]<<8)|((unsigned)h[6]<<16)|((unsigned)h[7]<<24);audio_state[8]++;
  }else if(errno!=ENOENT){engine_state[19]++;return CPYMO_ERR_CAN_NOT_OPEN_FILE;}
  if(!c->file){audio_cache_path(path,sizeof path,e->assetloader.gamedir,folder,name,".pcm",a,id);c->file=fopen(path,"rb");}
if(!c->file){if(errno==ENOENT&&known_missing_audio(e->assetloader.gamedir,folder,name)){c->name[0]=0;engine_state[44]++;return CPYMO_ERR_SUCC;}engine_state[19]++;return CPYMO_ERR_CAN_NOT_OPEN_FILE;}engine_state[42]++;}
 else {snprintf(path,sizeof path,"%s/%s/%s.pcm",e->assetloader.gamedir,folder,c->name);c->file=fopen(path,"rb");}
 c->loop=loop;
 if(!c->file){
  const char* ext=id==0?e->gameconfig.bgmformat:id==1?e->gameconfig.seformat:e->gameconfig.voiceformat;
  cpymo_package* pkg=id==1&&e->assetloader.use_pkg_se?&e->assetloader.pkg_se:id==2&&e->assetloader.use_pkg_voice?&e->assetloader.pkg_voice:NULL;
  c->offset=0;
  if(pkg){cpymo_package_index index;error_t err=cpymo_package_find(&index,pkg,name);if(err){engine_state[19]++;return err;}snprintf(path,sizeof path,"%s/%s/%s.pak",e->assetloader.gamedir,folder,folder);c->offset=index.file_offset;c->length=index.file_length;}
  else snprintf(path,sizeof path,"%s/%s/%s.%s",e->assetloader.gamedir,folder,c->name,ext);
  c->file=fopen(path,"rb");if(!c->file){engine_state[19]++;return CPYMO_ERR_CAN_NOT_OPEN_FILE;}
  if(!pkg){fseek(c->file,0,SEEK_END);c->length=ftell(c->file);}
  if(fseek(c->file,c->offset,SEEK_SET)){stop(c);return CPYMO_ERR_BAD_FILE_FORMAT;}
  c->remaining=c->length;c->phase=0;
  c->mp3=calloc(1,sizeof *c->mp3);if(!c->mp3){stop(c);return CPYMO_ERR_OUT_OF_MEM;}
  mad_stream_init(&c->mp3->stream);mad_frame_init(&c->mp3->frame);mad_synth_init(&c->mp3->synth);
  if(!decode_mp3(c)){stop(c);return CPYMO_ERR_BAD_FILE_FORMAT;}
  if(pkg)engine_state[22]++;else engine_state[23]++;
 }
 engine_state[16]++;return CPYMO_ERR_SUCC;
}
error_t cpymo_audio_bgm_play(cpymo_engine* e,cpymo_str n,bool loop){return play(e,n,0,loop);}
error_t cpymo_audio_se_play(cpymo_engine* e,cpymo_str n,bool loop){return play(e,n,1,loop);}
error_t cpymo_audio_vo_play(cpymo_engine* e,cpymo_str n){return play(e,n,2,false);}
void cpymo_audio_bgm_stop(cpymo_engine* e){if(e->audio)stop(&((Audio*)e->audio)->ch[0]);}
void cpymo_audio_se_stop(cpymo_engine* e){if(e->audio)stop(&((Audio*)e->audio)->ch[1]);}
void cpymo_audio_vo_stop(cpymo_engine* e){if(e->audio)stop(&((Audio*)e->audio)->ch[2]);}
const char* cpymo_audio_get_bgm_name(cpymo_engine* e){return e->audio?((Audio*)e->audio)->ch[0].name:"";}
const char* cpymo_audio_get_se_name(cpymo_engine* e){return e->audio?((Audio*)e->audio)->ch[1].name:"";}
error_t cpymo_audio_play_video(cpymo_engine* e,const char* path){(void)e;(void)path;return CPYMO_ERR_UNSUPPORTED;}
void cpymo_audio_copy_mixed_samples(void* dst,size_t len,cpymo_audio_system* s){(void)s;memset(dst,0,len);}
bool cpymo_audio_channel_get_samples(void** samples,size_t* len,size_t c,cpymo_audio_system* s){(void)s;(void)c;*samples=NULL;*len=0;return false;}
