/* v1.2.1 test: NDS/DS Lite IPL theme palette, no game-asset changes. */
#ifndef CPYMO_NDS_THEME_H
#define CPYMO_NDS_THEME_H
#include <calico/nds/env.h>
/* NITRO Programming Manual, IPL favorite colors: 5-bit RGB 0..15. */
static const unsigned char nds_ipl_theme_rgb[16][3]={
 {12,16,19},{23,9,0},{31,0,3},{31,17,31},
 {31,18,0},{30,28,0},{21,31,0},{0,31,0},
 {0,20,7},{9,27,17},{6,23,30},{0,11,30},
 {0,0,18},{17,0,26},{26,0,29},{31,0,18}
};
static const char* const nds_ipl_theme_names[4][16]={
 {"灰蓝","棕色","红色","粉色","橙色","黄色","青柠","绿色","深绿","青绿","浅蓝","蓝色","深蓝","深紫","淡紫","洋红"},
 {"Grey blue","Brown","Red","Pink","Orange","Yellow","Lime","Green","Dark green","Sea green","Cyan","Blue","Dark blue","Purple","Violet","Magenta"},
 {"灰藍","棕色","紅色","粉色","橘色","黃色","青檸","綠色","深綠","青綠","淺藍","藍色","深藍","深紫","淡紫","洋紅"},
 {"グレー","ブラウン","赤","ピンク","オレンジ","黄色","ライム","緑","深緑","青緑","水色","青","濃紺","紫","薄紫","マゼンタ"}
};
/* 0=original default; 1=firmware Auto; 2..17=the sixteen IPL colors. */
static unsigned nds_theme_preference;
static int nds_theme_effective_index=-1;
typedef struct {
 u16 bg,header,card,selected,accent,primary,muted,detail,clock,aux_clock,rail;
} NdsThemeScheme;
static NdsThemeScheme nds_theme_scheme;

/* Firmware is read only at boot or explicit theme changes, never on a pixel. */
static unsigned nds_theme_firmware(void){
 if(g_envUserSettings->version!=5||PersonalData->settingsLost||PersonalData->theme>15)return 16;
 return PersonalData->theme;
}
static u16 nds_theme_tint(unsigned idx,unsigned strength,unsigned neutral){
 const unsigned char* c=nds_ipl_theme_rgb[idx];
 return RGB15(neutral+(c[0]*strength+15)/31,
              neutral+(c[1]*strength+15)/31,
              neutral+(c[2]*strength+15)/31);
}
static void nds_theme_resolve(void){
 int idx=-1;
 if(nds_theme_preference==1){
  unsigned firmware=nds_theme_firmware();
  if(firmware<16)idx=(int)firmware;
 }else if(nds_theme_preference>=2&&nds_theme_preference<=17){
  idx=(int)nds_theme_preference-2;
 }
 nds_theme_effective_index=idx;
 if(idx<0){
  /* Keep original default color roles intact; B rail is the only change. */
  nds_theme_scheme=(NdsThemeScheme){
   RGB15(3,5,8),RGB15(5,10,16),RGB15(4,7,11),RGB15(7,15,23),
   RGB15(10,24,31),RGB15(31,31,31),RGB15(19,22,25),
   RGB15(26,27,29),RGB15(26,28,30),RGB15(21,24,27),
   RGB15(15,26,30)};
 }else{
  unsigned i=(unsigned)idx;
  /* Every role derives from the original IPL hue, never a blue base.
   * All color sums fit RGB555 without overflowing a 5-bit component. */
  nds_theme_scheme=(NdsThemeScheme){
   nds_theme_tint(i,6,1),nds_theme_tint(i,11,3),
   nds_theme_tint(i,9,3),nds_theme_tint(i,11,4),
   /* Approved A typography: neutral primary, hue-aware accents,
    * softened secondary/detail/clock ink. Keep every surface and rail. */
   nds_theme_tint(i,13,17),RGB15(30,30,30),
   nds_theme_tint(i,3,23),nds_theme_tint(i,3,24),
   nds_theme_tint(i,2,24),nds_theme_tint(i,3,23),
   nds_theme_tint(i,13,13)};
 }
}
static int nds_theme_active_index(void){return nds_theme_effective_index;}
static void nds_theme_load(void){
 nds_theme_preference=0;
 FILE* f=fopen("/pymo/nds-theme.cfg","rb");
 if(f){
  char line[32]={0};unsigned v=18;int n=0;
  if(fgets(line,sizeof line,f)&&sscanf(line,"NDSTHEME1 %u %n",&v,&n)==1
       && n>0&&line[n]==0&&fgetc(f)==EOF&&v<=17)nds_theme_preference=v;
  fclose(f);
 }
 nds_theme_resolve();
}
static int nds_theme_set(unsigned v){
 if(v>17)return 0;
 mkdir("/pymo",0777);FILE* f=fopen("/pymo/nds-theme.cfg","wb");int ok=0;
 if(f){ok=fprintf(f,"NDSTHEME1 %u\n",v)>0;if(fclose(f))ok=0;}
 if(ok){nds_theme_preference=v;nds_theme_resolve();}
 return ok;
}
static unsigned nds_theme_get(void){return nds_theme_preference;}
static u16 nds_theme_color(unsigned id){
 if(id>15)id=0;
 return RGB15(nds_ipl_theme_rgb[id][0],nds_ipl_theme_rgb[id][1],nds_ipl_theme_rgb[id][2]);
}
static const char* nds_theme_choice_name(unsigned v){
 if(v==0)return nds_ui("默认");
 if(v==1)return nds_ui("自动");
 if(v>17)return "?";
 return nds_ipl_theme_names[nds_language_effective()][v-2];
}
/* Constant-time palette lookups, including when the preference is Auto. */
static u16 nds_theme_bg(void){return nds_theme_scheme.bg;}
static u16 nds_theme_header(void){return nds_theme_scheme.header;}
static u16 nds_theme_card(void){return nds_theme_scheme.card;}
static u16 nds_theme_selected(void){return nds_theme_scheme.selected;}
static u16 nds_theme_accent(void){return nds_theme_scheme.accent;}
static u16 nds_theme_primary(void){return nds_theme_scheme.primary;}
static u16 nds_theme_muted(void){return nds_theme_scheme.muted;}
static u16 nds_theme_detail(void){return nds_theme_scheme.detail;}
static u16 nds_theme_clock_ink(void){return nds_theme_scheme.clock;}
static u16 nds_theme_aux_clock_ink(void){return nds_theme_scheme.aux_clock;}
static u16 nds_theme_rail(void){return nds_theme_scheme.rail;}
#endif
