/* NDS per-game preferences; AGPL-3.0-or-later. */
static int nds_read_gray=1;
static int nds_chara_effects=1;
/* magic, enabled, instant transition batches, writes, write errors */
volatile unsigned chara_effect_state[5]={0x4e434658,1};
static bool nds_playing,nds_aux_override,nds_touch_blocked,error_ui_active;
/* magic, gray, policy, hidden, backlight mask, writes, errors, touch wakes,
   Y toggles, loaded, textbox alpha/1000, UI open */
volatile unsigned ui_preferences_state[12]={0x4e555031};
static void nds_aux_refresh(void);
static void launcher_toast(const char* message);
static void nds_global_preferences_load(void){
 nds_chara_effects=1;FILE* f=fopen("/pymo/nds-global.cfg","rb");
 if(f){char line[64];int value,n=0;
  if(fgets(line,sizeof line,f)&&sscanf(line,"NDSGLOBAL1 %d %n",&value,&n)==1&&n>0&&line[n]==0&&fgetc(f)==EOF&&value>=0&&value<=1)nds_chara_effects=value;
  fclose(f);
 }chara_effect_state[1]=nds_chara_effects;
}
int nds_chara_effects_get(void){return nds_chara_effects;}
void nds_chara_effects_set(int value){
 if(value<0||value>1)return;nds_chara_effects=value;chara_effect_state[1]=value;
 FILE* f=fopen("/pymo/nds-global.cfg","wb");bool ok=false;
 if(f){ok=fprintf(f,"NDSGLOBAL1 %d\n",value)>0;if(fclose(f))ok=false;}
 chara_effect_state[3]++;if(!ok){chara_effect_state[4]++;launcher_toast(nds_ui("设置保存失败"));}
}
static bool nds_aux_hidden(void){return nds_playing&&!error_ui_active&&nds_aux_override;}
static void nds_preferences_sync(void){
 ui_preferences_state[1]=nds_read_gray;ui_preferences_state[2]=engine.say.current_say_is_already_read;
 ui_preferences_state[3]=nds_aux_hidden();
 ui_preferences_state[10]=(unsigned)(engine.say.textbox.alpha*1000+.5f);
 ui_preferences_state[11]=cpymo_ui_enabled(&engine);
}
static void nds_preferences_load(const char* dir){
 nds_read_gray=1;nds_aux_override=false;nds_touch_blocked=false;
 ui_preferences_state[9]=0;
 char path[512],line[64];snprintf(path,sizeof path,"%s/save/nds-ui.cfg",dir);
 FILE* f=fopen(path,"rb");if(f){int gray,n=0;
  if(fgets(line,sizeof line,f)&&sscanf(line,"NDSUI1 %d %n",&gray,&n)==1&&n>0&&line[n]==0&&fgetc(f)==EOF&&gray>=0&&gray<=1){nds_read_gray=gray;ui_preferences_state[9]=1;}
  fclose(f);
 }nds_preferences_sync();
}
static void nds_preferences_save(void){
 char path[512];snprintf(path,sizeof path,"%s/save/nds-ui.cfg",engine.assetloader.gamedir);
 FILE* f=nds_save_ensure_directory(engine.assetloader.gamedir)?fopen(path,"wb"):NULL;bool ok=false;if(f){ok=fprintf(f,"NDSUI1 %d\n",nds_read_gray)>0;if(fclose(f))ok=false;}
 ui_preferences_state[5]++;if(!ok){ui_preferences_state[6]++;launcher_toast(nds_ui("设置保存失败"));}nds_preferences_sync();
}
int nds_read_gray_get(void){return nds_read_gray;}
void nds_read_gray_set(int value){
 if(value<0||value>1)return;nds_read_gray=value;
 engine.say.textbox.alpha=engine.say.current_say_is_already_read&&value?.6f:1.f;
 nds_preferences_save();cpymo_engine_request_redraw(&engine);
}
