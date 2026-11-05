/* Experimental NVL1 RGB555/LZ4 delta video; AGPL-3.0-or-later. */
/* An NVL1/NVA1 movie takes over the upper game LCD directly. In enhanced
 * dual-screen mode, the physical lower LCD otherwise retains the prior
 * dialogue because cpymo_engine_draw does not run during movie playback.
 * Render ONLY the owner-approved illustration, without dialogue or the
 * "continue" hint, once before the first movie frame.
 *
 * Do not touch VRAM A (upper movie), audio scheduling, touch mapping or the
 * per-game mode. The normal end/skip/error cleanup requests an engine redraw
 * that will restore the ordinary lower-screen story or menu afterward.
 */
static void nds_movie_prepare_lower_screen(void){
 if(!nds_dual_active() || !launcher_sub)return;
 if(!nds_picture_draw(framebuffer,false)){
  /* Unpatched developer ROM: use the prior decorative fallback, without
     synthesizing a story prompt or repeating stale dialogue. */
  nds_dual_canvas_draw_yield(framebuffer,false,false,
                             nds_dual_canvas_audio_tick,NULL);
 }
 DC_FlushRange(framebuffer,sizeof framebuffer);
 dmaCopy(framebuffer,launcher_sub,sizeof framebuffer);
}
volatile unsigned movie_state[24]={0x4e564c37};
extern bool nds_movie_audio_begin(const char*,unsigned,unsigned);
extern void nds_movie_audio_verify(bool);
extern bool nds_movie_audio_begin_nar(const char*,unsigned,unsigned);
extern unsigned nds_movie_audio_position(void);
extern void nds_movie_audio_end(void);
static bool nvl_read(FILE* f,void* dst,unsigned n){
 unsigned done=0;while(done<n){unsigned k=n-done;if(k>8192)k=8192;if(fread((unsigned char*)dst+done,1,k,f)!=k)return false;done+=k;nds_audio_pump_if_due();}return true;
}
static void nvl_research_log(cpymo_engine* e,cpymo_str name,const char* status,
 unsigned frames_total,unsigned decoded,unsigned displayed,unsigned samples,
 unsigned elapsed_ms,unsigned late,unsigned error_code){
 FILE* enabled=savefile(e->assetloader.gamedir,"nds-nvl-probe.flag","rb");
 if(!enabled)return;
 fclose(enabled);
 char safe[33]={0},report[80];unsigned j=0;
 for(unsigned i=0;i<name.len&&j<32;i++){
  unsigned char c=(unsigned char)name.begin[i];
  safe[j++]=((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
              (c>='0'&&c<='9')||c=='_'||c=='-')?(char)c:'_';
 }
 if(!j){safe[0]='_';safe[1]=0;}
 snprintf(report,sizeof report,"nds-nvl-probe-%s.txt",safe);
 FILE* log=savefile(e->assetloader.gamedir,report,"wb");
 if(!log)return;
 fprintf(log,"CPyMO 1.2 NVL1/NVA1 playback diagnostic\n");
 fprintf(log,"mode=%s\n",isDSiMode()?"DSi-TWL":"DS-NTR");
 fprintf(log,"status=%s error=%u\n",status,error_code);
 fprintf(log,"frames_decoded=%u/%u frames_presented=%u/%u late=%u\n",
         decoded,frames_total,displayed,frames_total,late);
 fprintf(log,"audio_samples=%u elapsed_ms=%u\n",samples,elapsed_ms);
 fclose(log);
}
error_t __wrap_cpymo_movie_play(cpymo_engine* e,cpymo_str name){
 /* Probe is strictly opt-in and non-decoding; regular NVL playback is unchanged. */
 if(nds_fvds_preflight(e,name))return CPYMO_ERR_SUCC;
 u64 movie_begin=tickGetCount();unsigned decoded_before=movie_state[3],presented_before=movie_state[4],late_before=movie_state[5];
 FILE* audio_verify=savefile(e->assetloader.gamedir,"nds-video-verify.flag","rb");nds_movie_audio_verify(audio_verify!=NULL);if(audio_verify)fclose(audio_verify);
 movie_state[2]++;char path[512];nds_cache_path(path,sizeof path,e->assetloader.gamedir,"video",name,".nvl");
 FILE* f=fopen(path,"rb");if(!f){
  movie_state[8]++;engine_state[45]++;
  nvl_research_log(e,name,"MISSING",0,0,0,0,0,0,1);
  return CPYMO_ERR_SUCC;
 }
 unsigned h[8];bool header_read=fread(h,4,8,f)==8;bool compressed=header_read&&!memcmp(h,"NVA1",4);bool valid=header_read&&(!memcmp(h,"NVL1",4)||compressed)&&h[1]==256&&h[2]==192&&h[3]>0&&h[3]<=30&&h[4]>0&&h[4]<=h[3]*600&&h[5]==22050&&h[6]==(unsigned long long)h[4]*22050/h[3]&&h[7]>=32+h[4]*12&&h[7]<512*1024*1024;
 if(valid){long pos=ftell(f);valid=!fseek(f,0,SEEK_END);long size=valid?ftell(f):-1;if(valid)valid=compressed?(size>=(long)(h[7]+16)&&size<=(long)(h[7]+16+h[6]*2+((h[6]+1023)/1024)*12)):(size==(long)(h[7]+h[6]*2));valid=valid&&!fseek(f,pos,SEEK_SET);}
 unsigned char *packet=valid?malloc(98304):NULL,*delta=packet?malloc(98304):NULL;
 bool started=false,failed=!packet||!delta,skipped=false;unsigned last_samples=0,audio_errors_before=movie_state[7];
 if(failed)goto done;
 /* Movie accepted and buffers acquired: clear the previous dialogue from
    the lower LCD exactly once, before starting frame/audio playback. */
 nds_movie_prepare_lower_screen();
 memset(framebuffer,0,sizeof framebuffer);movie_state[1]=1;movie_state[19]=h[4];movie_state[11]=(unsigned long long)h[4]*1000/h[3];
 for(unsigned frame=0;frame<h[4];frame++){
  u64 start=tickGetCount();unsigned p[3];if(!nvl_read(f,p,12)||!p[0]||p[0]>98304||(p[1]&~3)||(!frame&&(p[1]&2))||!nvl_read(f,packet,p[0])){failed=true;break;}
  if(p[1]&1){if(!npl_decode(packet,p[0],delta,98304)){failed=true;break;}}else{if(p[0]!=98304){failed=true;break;}memcpy(delta,packet,98304);}
  if(p[1]&2){u32* a=(u32*)framebuffer;u32* b=(u32*)delta;for(unsigned i=0;i<98304/4;i++)a[i]^=b[i];}else memcpy(framebuffer,delta,98304);
  if(npl_crc((unsigned char*)framebuffer,98304)!=p[2]){failed=true;break;}
  unsigned us=(tickGetCount()-start)*1000000/TICK_FREQ;if(us>movie_state[9])movie_state[9]=us;movie_state[3]++;
  if(frame){while((unsigned long long)nds_movie_audio_position()*h[3]<(unsigned long long)frame*22050){nds_power_poll();scanKeys();if(keysDown()&KEY_B){skipped=true;goto done;}nds_audio_update();swiWaitForVBlank();}}
  nds_audio_update();DC_FlushRange(framebuffer,sizeof framebuffer);dmaCopy(framebuffer,vram,sizeof framebuffer);movie_state[4]++;movie_state[15]=frame;
  if(!started){if(!(compressed?nds_movie_audio_begin_nar(path,h[7],h[6]):nds_movie_audio_begin(path,h[7],h[6]))){failed=true;break;}started=true;}
  if(movie_state[7]!=audio_errors_before){failed=true;break;}
  unsigned samples=nds_movie_audio_position();unsigned target=(unsigned long long)frame*22050/h[3];unsigned late=samples>target?(unsigned long long)(samples-target)*1000000/22050:0;
  if(late>movie_state[16])movie_state[16]=late;if(late>1000000/h[3])movie_state[5]++;
  if(late>500000){failed=true;break;}
 }
 if(started&&!failed&&!skipped)while(nds_movie_audio_position()<h[6]){nds_power_poll();scanKeys();if(keysDown()&KEY_B){skipped=true;break;}nds_audio_update();swiWaitForVBlank();}
done:
 if(movie_state[7]!=audio_errors_before)failed=true;
 if(started){last_samples=nds_movie_audio_position();nds_movie_audio_end();}movie_state[18]=last_samples;movie_state[10]=(unsigned long long)last_samples*1000/22050;
 if(failed){movie_state[7]++;engine_state[45]++;}else if(skipped)movie_state[12]++;else movie_state[6]++;
 nvl_research_log(e,name,failed?"ERROR":skipped?"SKIPPED":"EOF",
  valid?h[4]:0,movie_state[3]-decoded_before,movie_state[4]-presented_before,
  last_samples,(unsigned)((tickGetCount()-movie_begin)*1000ULL/TICK_FREQ),
  movie_state[5]-late_before,failed?1:0);
 free(delta);free(packet);fclose(f);movie_state[1]=0;movie_state[21]++;loaded_image=true;cpymo_engine_request_redraw(e);return CPYMO_ERR_SUCC;
}
