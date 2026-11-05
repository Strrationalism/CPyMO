/* DS front end, AGPL-3.0-or-later. Included after the shared font renderer. */
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <unistd.h>
#include "app-icon.h"
typedef struct {char path[256],title[256],platform[8];unsigned status;u16 width,height;bool title_ok,meta_ok,icon_ok,list_title_ok;char intro[2][128];unsigned char title_bits[350],meta_bits[1050],list_title_bits[400];u16 icon[24*24];} LauncherGame;
static LauncherGame launcher_games[64];
/* magic, visible, count, selected, launches, scan truncated, last init error */
volatile unsigned launcher_state[7]={0x4c41554e};
static unsigned launcher_count,launcher_selected;
/* Four visible rows; keep the selected game on-screen without page jumps. */
static unsigned launcher_scroll_first;
static bool launcher_scroll_dragging;
static int launcher_scroll_drag_y;
static unsigned launcher_scroll_drag_first;
static void launcher_scroll_ensure_visible(void){
 if(launcher_count<=4){launcher_scroll_first=0;return;}
 unsigned last_start=launcher_count-4;
 if(launcher_scroll_first>last_start)launcher_scroll_first=last_start;
 if(launcher_selected<launcher_scroll_first)launcher_scroll_first=launcher_selected;
 else if(launcher_selected>=launcher_scroll_first+4)launcher_scroll_first=launcher_selected-3;
}
static u16* launcher_sub;
static bool launcher_about,launcher_restored,launcher_language_menu,launcher_theme_menu;
static unsigned launcher_theme_cursor;
static u64 launcher_refresh_notice_until;
/* User-facing text describes the *runtime mode*, not a guessed shell
 * model: DSi and DSi XL/LL share the same TWL mode; a DSi can also run
 * homebrew in NTR/DS mode. 16/4 MiB are hardware RAM capacities, not free
 * heap sizes. Detailed heap bounds stay available in nds_runtime_state. */
static const char* nds_runtime_mode_label(void){
 return nds_ui(isDSiMode() ? "DSi 模式 · 16 MiB" : "DS 模式 · 4 MiB");
}
static unsigned launcher_language_cursor;
static char launcher_font_path[256],launcher_icon_path[256];
static u16 selected_icon[48*48];static bool selected_icon_ok;
static u64 toast_started;
/* Enhanced split cannot show the classic aux-panel toast on its touch LCD. */
static char nds_toast_text[128];
/* magic, about, font reads, title reads, restore success, selection writes, write errors */
volatile unsigned frontend_state[7]={0x4e465231};
/* magic, visible, presentations, completed library presentations */
volatile unsigned splash_state[4]={0x4e53504c};
static void launcher_remember(void){
 if(!launcher_count)return;mkdir("/pymo",0777);FILE* f=fopen("/pymo/last-game.txt","wb");bool ok=false;
 if(f){ok=fprintf(f,"%s\n",launcher_games[launcher_selected].path)>0;if(fclose(f))ok=false;}
 frontend_state[5]++;if(!ok)frontend_state[6]++;
}
static bool launcher_file(const char* dir,const char* suffix){char path[512];struct stat s;snprintf(path,sizeof path,"%s/%s",dir,suffix);return !stat(path,&s)&&S_ISREG(s.st_mode);}
/* Explicit device-prefixed paths let the unmodified CPyMO asset loader,
 * config and save code operate on either mounted FAT volume. Never rewrite
 * one game's source paths or redirect its save files. */
static bool launcher_valid_game_path(const char* dir){
 return dir && (dir[0]=='/' ||
                 !strncasecmp(dir,"fat:/",5) ||
                 !strncasecmp(dir,"sd:/",4));
}
static void launcher_add(const char* dir){
 if(!launcher_valid_game_path(dir)||strlen(dir)>=256||!launcher_file(dir,"gameconfig.txt"))return;
 for(unsigned i=0;i<launcher_count;i++)if(!strcasecmp(launcher_games[i].path,dir))return;
 if(launcher_count==64){launcher_state[5]=1;return;}
 LauncherGame* game=&launcher_games[launcher_count++];memset(game,0,sizeof *game);snprintf(game->path,sizeof game->path,"%s",dir);
 cpymo_gameconfig config;char path[512];snprintf(path,sizeof path,"%s/gameconfig.txt",dir);
 bool parsed=!cpymo_gameconfig_parse_from_file(&config,path);
 if(parsed){snprintf(game->title,sizeof game->title,"%s",config.gametitle);snprintf(game->platform,sizeof game->platform,"%s",config.platform);game->width=config.imagesize_w;game->height=config.imagesize_h;}
 else game->status=3;
 char* line=game->title;for(unsigned i=0;i<3;i++){
 char* next=strstr(line,"\\n");unsigned delimiter=2;if(!next){next=strchr(line,'\n');delimiter=1;}
 if(next)*next=0;if(i)snprintf(game->intro[i-1],sizeof game->intro[0],"%s",line);
 if(!next)break;line=next+delimiter;
 }
 if(!game->title[0])snprintf(game->title,sizeof game->title,"%s",strrchr(dir,'/')+1);
 if(!parsed)return;snprintf(path,sizeof path,"%s/nds-cache/format.txt",dir);FILE* f=fopen(path,"rb");if(f){char marker[32]={0};fread(marker,1,31,f);fclose(f);game->status=(!strcmp(marker,"CPYMO-NDS-CACHE-1\n")||!strcmp(marker,"CPYMO-NDS-CACHE-2\n")||!strcmp(marker,"CPYMO-NDS-CACHE-3\n")||!strcmp(marker,"CPYMO-NDS-CACHE-4\n")||!strcmp(marker,"CPYMO-NDS-CACHE-5\n")||!strcmp(marker,"CPYMO-NDS-CACHE-6\n"))&&launcher_file(dir,"nds-cache/font.bin")?1:2;}
 snprintf(path,sizeof path,"%s/nds-cache/launcher-title.bin",dir);f=fopen(path,"rb");if(f){
 struct {char magic[4];u16 w,h;} h;game->title_ok=fread(&h,1,8,f)==8&&!memcmp(h.magic,"NLT1",4)&&h.w==200&&h.h==14&&fread(game->title_bits,1,350,f)==350&&fgetc(f)==EOF;fclose(f);frontend_state[3]++;}
 snprintf(path,sizeof path,"%s/nds-cache/launcher-meta.bin",dir);f=fopen(path,"rb");if(f){
 struct {char magic[4];u16 w,h;} h;game->meta_ok=fread(&h,1,8,f)==8&&!memcmp(h.magic,"NLM1",4)&&h.w==200&&h.h==42&&fread(game->meta_bits,1,1050,f)==1050&&fgetc(f)==EOF;fclose(f);
 if(game->meta_ok){memcpy(game->title_bits,game->meta_bits,350);game->title_ok=true;}}
 snprintf(path,sizeof path,"%s/nds-cache/launcher-icon.bin",dir);f=fopen(path,"rb");if(f){
 struct {char magic[4];u16 w,h;} h;u16 row[48];bool ok=fread(&h,1,8,f)==8&&!memcmp(h.magic,"NLI1",4)&&h.w==48&&h.h==48;
 for(unsigned y=0;ok&&y<48;y++){ok=fread(row,2,48,f)==48;if(!(y&1))for(unsigned x=0;x<24;x++)game->icon[(y/2)*24+x]=row[x*2];}
 game->icon_ok=ok&&fgetc(f)==EOF;fclose(f);}


}
static void launcher_scan_dir(const char* root){
 DIR* d=opendir(root);if(!d)return;struct dirent* entry;char path[256];unsigned scanned=0;
 while((entry=readdir(d))&&scanned++<1024){if(entry->d_name[0]=='.')continue;int n=snprintf(path,sizeof path,"%s%s%s",root,root[strlen(root)-1]=='/'?"":"/",entry->d_name);if(n>0&&n<(int)sizeof path)launcher_add(path);}
 if(scanned>=1024)launcher_state[5]=1;closedir(d);
}
/* libfat's fatInitDefault mounts both "sd:" and "fat:" where available,
 * then chooses the running executable's device as cwd. Preserve the current
 * /pymo/games scan and scan only the *other* device to avoid duplicates.
 * DS/DS Lite never attempts DSi-SD discovery. No additional card writes. */
static void launcher_scan_other_card(void){
 if(!isDSiMode())return;
 char cwd[256];
 if(!getcwd(cwd,sizeof cwd))return;
 const char* other=NULL;
 if(!strncasecmp(cwd,"fat:",4))other="sd:";
 else if(!strncasecmp(cwd,"sd:",3))other="fat:";
 if(!other)return;
 char path[32];
 snprintf(path,sizeof path,"%s/pymo/games",other);
 launcher_scan_dir(path);
 snprintf(path,sizeof path,"%s/",other);
 launcher_scan_dir(path);
}
static int launcher_compare(const void* a,const void* b){return strcasecmp(((const LauncherGame*)a)->path,((const LauncherGame*)b)->path);}
static void launcher_scan(const char* hint){
 char previous[256]={0};if(launcher_count)snprintf(previous,sizeof previous,"%s",launcher_games[launcher_selected].path);
 launcher_count=0;launcher_state[5]=0;launcher_scan_dir("/pymo/games");launcher_scan_dir("/");launcher_scan_other_card();launcher_add(hint);
 const char* configs[]={"/cpymo-nds/cache-game-path.txt","/cpymo-nds/game-path.txt"};
 for(unsigned i=0;i<2;i++){FILE* f=fopen(configs[i],"rb");if(f){char path[256];if(fgets(path,sizeof path,f)){path[strcspn(path,"\r\n")]=0;launcher_add(path);}fclose(f);}}
 qsort(launcher_games,launcher_count,sizeof *launcher_games,launcher_compare);if(launcher_selected>=launcher_count)launcher_selected=0;launcher_state[2]=launcher_count;
 if(!launcher_restored){FILE* f=fopen("/pymo/last-game.txt","rb");if(f){if(fgets(previous,sizeof previous,f))previous[strcspn(previous,"\r\n")]=0;fclose(f);}launcher_restored=true;}
 for(unsigned i=0;i<launcher_count;i++)if(!strcmp(previous,launcher_games[i].path)){launcher_selected=i;frontend_state[4]=1;break;}

}
static void launcher_rect(int x,int y,int w,int h,u16 color){for(int yy=y;yy<y+h&&yy<192;yy++)for(int xx=x;xx<w+x&&xx<256;xx++)if(xx>=0&&yy>=0)framebuffer[yy*256+xx]=color|BIT(15);}
/* Preserve R5's full-width game rows and 4px visual track. The 14px
 * touch target is hit testing only (partly overlapping the visually blank
 * right edge of a row), so increased usability does not change appearance. */
#define LAUNCHER_SCROLL_Y 33
#define LAUNCHER_SCROLL_H 121
#define LAUNCHER_SCROLL_TOUCH_X 242
static int launcher_scroll_thumb_height(void){
 if(launcher_count<=4)return LAUNCHER_SCROLL_H;
 int h=(LAUNCHER_SCROLL_H*4+(int)launcher_count/2)/(int)launcher_count;
 return h<14?14:h;
}
static int launcher_scroll_thumb_top(void){
 if(launcher_count<=4)return LAUNCHER_SCROLL_Y;
 int travel=LAUNCHER_SCROLL_H-launcher_scroll_thumb_height();
 unsigned max_first=launcher_count-4;
 return LAUNCHER_SCROLL_Y+(int)((travel*launcher_scroll_first+max_first/2)/max_first);
}
static bool launcher_scroll_set_first(unsigned first){
 if(launcher_count<=4)return false;
 const unsigned max_first=launcher_count-4;
 if(first>max_first)first=max_first;
 const unsigned old_first=launcher_scroll_first,old_selected=launcher_selected;
 launcher_scroll_first=first;
 /* Keep the highlighted title attached to a visible row, not an
  * offscreen game, when the viewport moves independently of the D-pad. */
 if(launcher_selected<first)launcher_selected=first;
 else if(launcher_selected>=first+4)launcher_selected=first+3;
 return old_first!=launcher_scroll_first||old_selected!=launcher_selected;
}
static bool launcher_scrollbar_begin_touch(int x,int y){
 if(launcher_count<=4||x<LAUNCHER_SCROLL_TOUCH_X||x>=256||
    y<LAUNCHER_SCROLL_Y||y>=LAUNCHER_SCROLL_Y+LAUNCHER_SCROLL_H)return false;
 int top=launcher_scroll_thumb_top(),height=launcher_scroll_thumb_height();
 /* A tap outside the thumb jumps to the corresponding place. Pressing
  * the thumb itself keeps its position and avoids an initial jump. */
 if(y<top||y>=top+height){
  int travel=LAUNCHER_SCROLL_H-height,relative=y-LAUNCHER_SCROLL_Y-height/2;
  if(relative<0)relative=0;
  if(relative>travel)relative=travel;
  launcher_scroll_set_first((unsigned)((relative*(int)(launcher_count-4)+travel/2)/travel));
 }
 launcher_scroll_dragging=true;
 launcher_scroll_drag_y=y;
 launcher_scroll_drag_first=launcher_scroll_first;
 return true;
}
static bool launcher_scrollbar_move_touch(int y){
 if(!launcher_scroll_dragging||launcher_count<=4)return false;
 const int travel=LAUNCHER_SCROLL_H-launcher_scroll_thumb_height();
 const int max_first=(int)launcher_count-4;
 const int delta=(y-launcher_scroll_drag_y)*max_first;
 /* Signed rounding, then clamp. Keep capture even if the pen slides
  * outside the narrow gutter until release, for natural nail dragging. */
 const int shift=(delta+(delta>=0?travel/2:-(travel/2)))/travel;
 int first=(int)launcher_scroll_drag_first+shift;
 if(first<0)first=0;
 if(first>max_first)first=max_first;
 return launcher_scroll_set_first((unsigned)first);
}
static void launcher_scrollbar_draw(void){
 if(launcher_count<=4)return;
 launcher_rect(251,LAUNCHER_SCROLL_Y,4,LAUNCHER_SCROLL_H,nds_theme_card());
 launcher_rect(252,launcher_scroll_thumb_top(),2,launcher_scroll_thumb_height(),nds_theme_accent());
}
static void launcher_text_mode(const char* text,int x,int y,u16 color,int max_x,unsigned mode){
 const char* p=text;int start=x;while(*p){unsigned cp=decode(&p);if(cp==13)continue;if(cp==10|| (cp=='\\'&&*p=='n')){if(cp=='\\')p++;x=start;y+=17;continue;}if(y>178)break;
 int g=text_glyph(mode,cp),fallback=g<0?ui_fallback_glyph(cp):-1;if(g<0&&fallback<0)g=text_glyph(mode,'?');if(g<0&&fallback<0)continue;unsigned advance=fallback>=0?ui_fallback_advances[fallback]:text_advance(mode,g);if(x+(int)advance>max_x){while(*p&&*p!='\n')p++;continue;}
 for(int yy=0;yy<(fallback>=0?16:(int)font_height(mode));yy++){unsigned bits=fallback>=0?ui_fallback_rows[fallback][yy]:text_row(mode,g,yy);for(int xx=0;xx<(fallback>=0?16:(int)font_width(mode));xx++)if(bits&(1u<<xx))launcher_rect(x+xx,y+yy,1,1,color);}x+=advance;}
}
static void launcher_text(const char* text,int x,int y,u16 color,int max_x){launcher_text_mode(text,x,y,color,max_x,nds_font_choices_available()?1:0);}
static int launcher_text_pixel_width(const char* text){
 unsigned mode=nds_font_choices_available()?1:0;int width=0;
 const char* p=text;
 while(*p){unsigned cp=decode(&p);int g=text_glyph(mode,cp);
  if(g<0)g=text_glyph(mode,'?');
  if(g>=0)width+=(int)text_advance(mode,g);
 }
 return width;
}
static void launcher_text_centered(const char* text,int cell_x,int cell_width,int y,u16 color){
 int x=cell_x+(cell_width-launcher_text_pixel_width(text))/2;
 if(x<cell_x+3)x=cell_x+3;
 launcher_text(text,x,y,color,cell_x+cell_width-3);
}
static void launcher_publish(u16* dest){DC_FlushRange(framebuffer,sizeof framebuffer);dmaCopy(framebuffer,dest,sizeof framebuffer);}
/* Keep both LCDs black while changing VRAM roles. Old console font/tile data
   must never be exposed as RGB555 pixels, including during FAT/library I/O. */
static void launcher_video_init(void){
 setBrightness(3,-16);lcdMainOnTop();screens_swapped=false;presentation_state[1]=0;
 videoSetMode(MODE_5_2D);vramSetBankA(VRAM_A_MAIN_BG);int main_bg=bgInit(3,BgType_Bmp16,BgSize_B16_256x256,0,0);vram=bgGetGfxPtr(main_bg);
 dmaFillHalfWords(BIT(15),vram,256*256*2);
 videoSetModeSub(MODE_5_2D);vramSetBankC(VRAM_C_SUB_BG);int sub_bg=bgInitSub(3,BgType_Bmp16,BgSize_B16_256x256,0,0);launcher_sub=bgGetGfxPtr(sub_bg);
 dmaFillHalfWords(BIT(15),launcher_sub,256*256*2);
}
static void launcher_video_show(void){swiWaitForVBlank();setBrightness(3,0);}
static bool launcher_title_bitmap(const LauncherGame* game,int x,int y,u16 color){
 if(!game->title_ok)return false;for(unsigned yy=0;yy<14;yy++)for(unsigned xx=0;xx<200;xx++)if(game->title_bits[yy*25+xx/8]&(1u<<(xx%8)))launcher_rect(x+xx,y+yy,1,1,color);return true;
}
static void launcher_list_title(LauncherGame* game,int x,int y,u16 color){
 /* Use the exact native 12px dialogue font, including its full glyph grid.
    Older on-card title/meta bitmaps may still contain a pre-pixel-font style.
    Generate once from this game's own subset, never the selected game's. */
 if(!game->list_title_ok){
  PixelFont own={0};
  if(read_pixel_font(&own,game->path,"small",12)){
   memset(game->list_title_bits,0,sizeof game->list_title_bits);const char* p=game->title;unsigned pen=0;
   while(*p){unsigned cp=decode(&p);int lo=0,hi=own.count-1,found=-1;
    while(lo<=hi){int mid=(lo+hi)/2;if(own.data[mid].code==cp){found=mid;break;}if(own.data[mid].code<cp)lo=mid+1;else hi=mid-1;}
    int fallback=found<0?ui_fallback_glyph(cp):-1;
    if(found<0&&fallback<0){fallback=ui_fallback_glyph('?');if(fallback<0)continue;}
    unsigned advance=found>=0?own.data[found].advance:ui_fallback_advances[fallback];if(pen+advance>200)break;
    for(unsigned yy=0;yy<16;yy++){unsigned bits=found>=0?own.data[found].rows[yy]:ui_fallback_rows[fallback][yy];
     for(unsigned xx=0;xx<16&&pen+xx<200;xx++)if(bits&(1u<<xx))game->list_title_bits[yy*25+(pen+xx)/8]|=1u<<((pen+xx)%8);}
    pen+=advance;
   }
   free(own.data);game->list_title_ok=true;
  }
 }
 if(game->list_title_ok){for(unsigned yy=0;yy<16;yy++)for(unsigned xx=0;xx<200;xx++)if(game->list_title_bits[yy*25+xx/8]&(1u<<(xx%8)))launcher_rect(x+xx,y+yy,1,1,color);}
 else if(!launcher_title_bitmap(game,x,y,color))launcher_text_mode(game->title,x,y,color,244,0);
}
/* Fit complete UTF-8 glyphs on one line, reserving dots when text overflows. */
static void launcher_line(const char* text,int x,int y,u16 color,int max_x,unsigned mode){
 const char* p=text;unsigned width=0;while(*p&&*p!='\n'&&*p!='\r'&&!(p[0]=='\\'&&p[1]=='n')){unsigned cp=decode(&p);int g=text_glyph(mode,cp);if(g<0)g=text_glyph(mode,'?');if(g>=0)width+=text_advance(mode,g);}
 if(width<=(unsigned)(max_x-x)){launcher_text_mode(text,x,y,color,max_x,mode);return;}
 int dot=text_glyph(mode,'.');unsigned dots=dot>=0?3*text_advance(mode,dot):0;int limit=max_x-x-(int)dots;
 char fitted[256];unsigned used=0;int pixels=0;p=text;
 while(*p&&*p!='\n'&&*p!='\r'&&!(p[0]=='\\'&&p[1]=='n')){const char* begin=p;unsigned cp=decode(&p);int g=text_glyph(mode,cp);if(g<0)g=text_glyph(mode,'?');unsigned advance=g>=0?text_advance(mode,g):0;unsigned bytes=p-begin;if(pixels+(int)advance>limit||used+bytes+4>sizeof fitted)break;memcpy(fitted+used,begin,bytes);used+=bytes;pixels+=advance;}
 memcpy(fitted+used,dots?"...":"",dots?4:1);launcher_text_mode(fitted,x,y,color,max_x,mode);
}
static void launcher_title(const LauncherGame* game,int x,int y,u16 color,int max_x){
 if(nds_font_choices_available())launcher_line(game->title,x,y,color,max_x,2);
 else if(!launcher_title_bitmap(game,x,y,color))launcher_line(game->title,x,y,color,max_x,0);
}
static void launcher_background(void){const u16 color=nds_theme_bg()|BIT(15);for(unsigned i=0;i<256*192;i++)framebuffer[i]=color;}
/* C scheme: no redundant bar across the entire top of the screen. */
static void launcher_header(const char* title){
 launcher_rect(0,0,256,28,nds_theme_header());
 launcher_text(title,10,7,nds_theme_primary(),248);
}
static void launcher_icon(int x,int y){for(int yy=0;yy<32;yy++)for(int xx=0;xx<32;xx++){u16 c=app_icon[yy*32+xx];if(c)launcher_rect(x+xx,y+yy,1,1,c);}}
/* ROM-resident glyphs work before FAT or a game's font cache is available. */
static void launcher_splash_text(const char* text,int y,bool large,u16 color){
 const char* p=text;unsigned width=0;while(*p){int g=ui_fallback_glyph(decode(&p));if(g>=0)width+=large?ui_large_advances[g]:ui_fallback_advances[g];}
 int x=(256-(int)width)/2;p=text;while(*p){int g=ui_fallback_glyph(decode(&p));if(g<0)continue;
 for(int yy=0;yy<16;yy++){unsigned bits=large?ui_large_rows[g][yy]:ui_fallback_rows[g][yy];for(int xx=0;xx<16;xx++)if(bits&(1u<<xx))launcher_rect(x+xx,y+yy,1,1,color);}
 x+=large?ui_large_advances[g]:ui_fallback_advances[g];}
}
static void launcher_show_splash(void){
 launcher_background();
 for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){u16 c=app_icon_splash[y*64+x];if(c)launcher_rect(96+x,38+y,1,1,c);}
 launcher_splash_text("CPyMO for NDS",121,true,nds_theme_primary());launcher_publish(vram);
 launcher_background();launcher_splash_text(nds_ui("正在读取游戏列表..."),88,false,nds_theme_active_index()<0?RGB15(28,29,31):nds_theme_detail());
 launcher_splash_text(nds_runtime_mode_label(),166,false,nds_theme_muted());launcher_publish(launcher_sub);
 /* Both planes are complete already. No timer, key wait or extra VBlank. */
 setBrightness(3,0);splash_state[1]=1;splash_state[2]++;
}
static void launcher_selected_icon(void){
 if(!launcher_count)return;LauncherGame* g=&launcher_games[launcher_selected];if(!strcmp(launcher_icon_path,g->path))return;
 snprintf(launcher_icon_path,sizeof launcher_icon_path,"%s",g->path);selected_icon_ok=false;
 char path[512];snprintf(path,sizeof path,"%s/nds-cache/launcher-icon.bin",g->path);FILE* f=fopen(path,"rb");if(f){
 struct {char magic[4];u16 w,h;} h;selected_icon_ok=fread(&h,1,8,f)==8&&!memcmp(h.magic,"NLI1",4)&&h.w==48&&h.h==48&&fread(selected_icon,2,48*48,f)==48*48&&fgetc(f)==EOF;fclose(f);}
 presentation_state[6]=selected_icon_ok;presentation_state[7]=1+(g->intro[0][0]!=0)+(g->intro[1][0]!=0);
}
static void launcher_game_icon(const LauncherGame* game,int x,int y,bool large){
 if(large&&selected_icon_ok){for(unsigned yy=0;yy<48;yy++)for(unsigned xx=0;xx<48;xx++){u16 c=selected_icon[yy*48+xx];if(c&BIT(15))launcher_rect(x+xx,y+yy,1,1,c);}}
 else if(!large&&game->icon_ok){for(unsigned yy=0;yy<24;yy++)for(unsigned xx=0;xx<24;xx++){u16 c=game->icon[yy*24+xx];if(c&BIT(15))launcher_rect(x+xx,y+yy,1,1,c);}}
 else if(large)launcher_icon(x+8,y+8);
 else {for(unsigned yy=0;yy<24;yy++)for(unsigned xx=0;xx<24;xx++){u16 c=app_icon_small[yy*24+xx];if(c)launcher_rect(x+xx,y+yy,1,1,c);}}
}
static void launcher_intro(const LauncherGame* game,unsigned line,int x,int y,u16 color){
 /* Small dialogue font on its native grid; title remains the visual anchor. */
 launcher_line(game->intro[line-1],x,y,color,245,nds_font_choices_available()?1:0);
}
static bool nds_aux_clock_suppressed;
static void nds_clock_patch_aux(bool show){
 if(debug_trace||!launcher_sub||nds_aux_hidden())return;
 for(unsigned y=96;y<114;y++)for(unsigned x=196;x<244;x++)launcher_sub[y*256+x]=nds_theme_bg()|BIT(15);
 if(show){nds_clock_paint_impl(launcher_sub,196,96,nds_theme_aux_clock_ink(),false);clock_state[7]++;}
}
static void nds_clock_sync_aux_visibility(void){
 bool suppress=nds_menu_clock_visible;
 if(suppress==nds_aux_clock_suppressed)return;
 nds_aux_clock_suppressed=suppress;nds_clock_patch_aux(!suppress);
}
static void launcher_play_panel(const char* message){
 /* Enhanced mode owns the sub LCD; never repaint it with the classic help UI. */
 if(nds_dual_active())return;
 if(nds_aux_hidden()){if(launcher_sub)dmaFillHalfWords(BIT(15),launcher_sub,256*192*2);return;}
 u16 white=nds_theme_primary(),muted=nds_theme_muted(),accent=nds_theme_accent();
 launcher_background();launcher_header("CPyMO for NDS   " CPYMO_NDS_VERSION);
 /* Physical touch is a native game coordinate only in classic-lower.
    Do not imply the help LCD is a universal virtual A button. D-pad
    story shortcuts use the physical controller's UP/DOWN labels.
    Keep the game icon at x=196,y=40 and clock at x=196,y=96 untouched. */
 if(nds_dual_title_allow_split(&nds_title_gate)){
  launcher_text(nds_ui("A / DOWN：继续"),12,32,white,183);
  launcher_text(nds_ui("UP：历史对话"),12,50,white,183);
  launcher_text(nds_ui("X：菜单 · B：返回"),12,68,white,183);
  launcher_text(nds_ui("B：按住隐藏文字"),12,86,white,183);
  launcher_text(nds_ui("Y：隐藏／唤回辅助屏"),12,104,white,183);
  launcher_text(nds_ui("R：快进 · L：切换显示模式"),12,122,white,245);
  launcher_text(nds_ui("上屏游戏 / 下屏游戏 / 增强双屏"),12,140,accent,245);
  launcher_text(nds_ui("START：快速保存  SELECT：快速读取"),12,158,muted,245);
 }else{
  launcher_text(nds_ui("标题菜单"),12,32,accent,183);
  launcher_text(nds_ui("UP / DOWN：选择项目"),12,50,white,183);
  launcher_text(nds_ui("A：确认"),12,68,white,183);
  launcher_text(nds_ui("B：返回"),12,86,white,183);
  launcher_text(nds_ui("Y：隐藏／唤回辅助屏"),12,104,white,183);
  launcher_text(nds_ui("L：切换显示模式"),12,122,white,245);
  const char* active=nds_display_mode==0?"当前显示：上屏游戏":
                     nds_display_mode==1?"当前显示：下屏游戏":"当前显示：增强双屏";
  launcher_text(nds_ui(active),12,158,accent,245);
 }
 if(launcher_count)launcher_game_icon(&launcher_games[launcher_selected],196,40,true);
 /* Keep the last help row useful without placeholder text: status overlays
    native runtime mode metadata and resets when its toast expires. */
 launcher_text(message&&*message?nds_ui(message):nds_runtime_mode_label(),
               12,176,message&&*message?accent:muted,245);
 nds_clock_sample();nds_aux_clock_suppressed=nds_menu_clock_visible;
 if(!nds_aux_clock_suppressed)nds_clock_paint_impl(framebuffer,196,96,nds_theme_aux_clock_ink(),false);clock_state[6]++;
 launcher_publish(launcher_sub);
}
static void nds_clock_poll(void){
 nds_clock_sample();if(!nds_clock_pending)return;nds_clock_pending=false;
 if(nds_dual_active())return; /* sub LCD belongs to enhanced dialogue/UI */
 if(nds_menu_clock_visible&&cpymo_ui_enabled(&engine))cpymo_engine_request_redraw(&engine);
 if(nds_aux_hidden()){clock_state[8]++;return;}
 if(!nds_menu_clock_visible)nds_clock_patch_aux(true);
}
static void launcher_toast(const char* message){
 const char* localized=nds_ui(message?message:"");
 snprintf(nds_toast_text,sizeof nds_toast_text,"%s",localized?localized:"");
 toast_started=tickGetCount();presentation_state[2]=1;presentation_state[3]++;presentation_state[5]=0;
 /* Only the split mode owns both screens as gameplay. Redraw a brief
    upper-scene feedback banner rather than destroying its lower dialogue. */
 if(nds_dual_active())cpymo_engine_request_redraw(&engine);
 else launcher_play_panel(nds_toast_text);
}
static void launcher_update_toast(void){
 if(!presentation_state[2])return;
 u64 elapsed=tickGetCount()-toast_started;presentation_state[5]=elapsed*1000/TICK_FREQ;
 if(elapsed>=2*TICK_FREQ){
  presentation_state[2]=0;presentation_state[4]++;
  /* Force repaint to remove the actual upper-scene overlay; relying on
     script activity can leave a stale success message over a static CG. */
  if(nds_dual_active())cpymo_engine_request_redraw(&engine);
  else launcher_play_panel(NULL);
 }
}
/* Error rendering is ROM-only: it never consults game fonts or FAT. */
volatile unsigned error_ui_state[8]={0x4e455231};
static unsigned launcher_rom_width(const char* text,bool large){
 unsigned w=0;while(*text&&*text!='\n'){unsigned cp=decode(&text);int g=ui_fallback_glyph(cp);if(g<0)g=ui_fallback_glyph('?');if(g>=0)w+=large?ui_large_advances[g]:ui_fallback_advances[g];}return w;
}
static void launcher_rom_box(const char* text,int x,int y,int width,int bottom,bool large,u16 color){
 int left=x,right=x+width;const char* p=text;unsigned lines=1;
 while(*p){unsigned cp=decode(&p);if(cp==13)continue;
  if(cp==10){x=left;y+=18;lines++;continue;}
  int g=ui_fallback_glyph(cp);if(g<0){error_ui_state[5]++;g=ui_fallback_glyph('?');}if(g<0)continue;
  unsigned advance=large?ui_large_advances[g]:ui_fallback_advances[g];
  if(x+(int)advance>right){x=left;y+=18;lines++;if(cp==' ')continue;}
  if(y+16>bottom){error_ui_state[6]++;break;}
  for(unsigned yy=0;yy<16;yy++){unsigned bits=large?ui_large_rows[g][yy]:ui_fallback_rows[g][yy];
   for(unsigned xx=0;xx<16;xx++)if((bits&(1u<<xx))&&x+(int)xx<right)launcher_rect(x+xx,y+yy,1,1,color);}
  x+=advance;
 }error_ui_state[7]+=lines;
}
static void launcher_rom_center(const char* text,int y,bool large,u16 color){
 unsigned width=launcher_rom_width(text,large);int x=width<=224?(256-(int)width)/2:16;
 launcher_rom_box(text,x,y,256-16-x,164,large,color);
}
static void launcher_failure_hint(const char* message,int error,const char* hint){
 splash_state[1]=0;bool fatal=!strcmp(message,"无法访问存储卡");
 /* Errors must remain readable after L swapped the game or Y hid its panel.
    This affects errors only; normal game screen routing is unchanged. */
 error_ui_active=true;
 if(nds_playing){lcdMainOnTop();screens_swapped=false;presentation_state[1]=0;nds_aux_refresh();}
 error_ui_state[1]=fatal;error_ui_state[2]=(unsigned)error;error_ui_state[3]=nds_language_effective();
 error_ui_state[5]=error_ui_state[6]=error_ui_state[7]=0;
 launcher_background();launcher_rect(0,0,256,28,nds_theme_header());
 launcher_rom_box("CPyMO for NDS",10,7,236,28,false,nds_theme_primary());launcher_icon(112,43);
 launcher_rom_center(nds_ui(message),90,true,nds_theme_primary());
 char code[64];snprintf(code,sizeof code,nds_ui("错误码：%d"),error);
 launcher_rom_center(code,143,false,RGB15(31,23,18));launcher_publish(vram);
 launcher_background();launcher_rect(0,0,256,28,nds_theme_header());
 launcher_rom_box(nds_ui("处理建议"),10,7,236,28,false,nds_theme_primary());
 const char* advice=fatal?"关机后重新插好TF卡。\n检查读卡与R4内核文件，再重试。\n此错误码不代表TF卡已损坏。":"请检查转换结果和游戏资源。\n原始错误码已保留，便于反馈。";
 launcher_rom_box(nds_ui(advice),16,46,224,155,false,nds_theme_active_index()<0?RGB15(27,28,30):nds_theme_detail());
 launcher_rom_box(nds_ui(hint),16,169,224,192,false,nds_theme_accent());launcher_publish(launcher_sub);error_ui_state[4]++;
}
static void launcher_failure(const char* message,int error){launcher_failure_hint(message,error,"按 B 返回游戏资料库");}
static void launcher_library_clock(u16* pixels,bool erase){
 /* Header-only minute patch: never re-read game metadata or repaint the list. */
 /* Center the time numerals on the same visible baseline as the heading.
     Only modify this 48x18 patch, leaving the R7 list geometry unchanged. */
 if(erase)for(int y=6;y<24;y++)for(int x=200;x<248;x++)pixels[y*256+x]=nds_theme_header()|BIT(15);
 nds_clock_paint_impl(pixels,200,6,nds_theme_clock_ink(),false);
}
static void launcher_draw(const char* message){
 launcher_scroll_ensure_visible();
 if(launcher_count&&launcher_games[launcher_selected].status==1&&strcmp(launcher_font_path,launcher_games[launcher_selected].path)){if(load_game_font(launcher_games[launcher_selected].path)&&load_pixel_fonts(launcher_games[launcher_selected].path)){snprintf(launcher_font_path,sizeof launcher_font_path,"%s",launcher_games[launcher_selected].path);frontend_state[2]++;}}
 u16 white=nds_theme_primary(),muted=nds_theme_muted(),accent=nds_theme_accent();
 launcher_selected_icon();
 launcher_background();launcher_header("CPyMO for NDS   " CPYMO_NDS_VERSION);
 if(launcher_about){launcher_icon(12,40);launcher_text("PyMO / CPyMO",53,47,white,246);launcher_text(nds_ui("NDS 移植：千岚之夏（Blueteemo）"),12,78,white,245);launcher_text(nds_ui("引擎与后端：AGPL v3"),12,101,muted,245);launcher_text(nds_ui("转换工具：MIT"),12,124,muted,245);launcher_text(nds_ui("许可详情请见附带文档"),12,147,muted,245);}
 else if(launcher_count){LauncherGame* game=&launcher_games[launcher_selected];launcher_game_icon(game,12,39,true);launcher_text(nds_ui("游戏资料库"),74,53,accent,245);
 launcher_title(game,12,90,white,245);
 launcher_intro(game,1,12,114,nds_theme_detail());launcher_intro(game,2,12,132,nds_theme_detail());
 const char* path=game->path;while(strlen(path)>36)path++;launcher_text(path,12,153,muted,215);
 if(!strncasecmp(game->path,"sd:/",4))launcher_text("SD",220,153,accent,248);
 else if(!strncasecmp(game->path,"fat:/",5))launcher_text("TF",220,153,accent,248);
 launcher_text(nds_ui(game->status==1?"已转换，可以启动":game->status==2?"缓存无效，请重新转换":game->status==3?"游戏配置无效":"需要先使用转换脚本"),12,175,accent,245);}
 else {launcher_text(nds_ui("没有找到游戏"),12,58,white,245);launcher_text(nds_ui("请先转换游戏，再放入"),12,88,muted,245);launcher_text(nds_ui("/pymo/games/"),12,114,accent,245);}
 if(!launcher_count||launcher_about)launcher_text(nds_runtime_mode_label(),12,177,muted,245);launcher_publish(vram);
 launcher_background();
 if(!launcher_about&&!launcher_language_menu&&!launcher_theme_menu){
  launcher_rect(0,0,256,28,nds_theme_header());
  launcher_line(nds_ui("选择游戏"),10,7,white,194,1);
  nds_clock_sample();launcher_library_clock(framebuffer,false);
 }else launcher_header(nds_ui(launcher_theme_menu?"主题设置":launcher_about?"关于与许可":"界面语言"));
 if(launcher_theme_menu){
  for(unsigned i=0;i<2;i++){
   int x=i?131:8,w=i?117:115;
   launcher_rect(x,33,w,22,launcher_theme_cursor==i?nds_theme_selected():nds_theme_card());
   launcher_text_centered(nds_theme_choice_name(i),x,w,36,white);
  }
  for(unsigned i=0;i<16;i++){
   int x=8+(i%4)*61,y=60+(i/4)*24;
   if(launcher_theme_cursor==i+2)launcher_rect(x-2,y-2,61,25,white);
   launcher_rect(x,y,57,21,nds_theme_color(i));
  }
  launcher_text(nds_theme_choice_name(launcher_theme_cursor),12,156,accent,245);
  launcher_text(nds_ui("方向键选择 · A 确定 · B 返回"),7,174,muted,250);
 }
 else if(launcher_language_menu){
 for(unsigned i=0;i<NDS_LANG_COUNT;i++){int y=34+i*25;launcher_rect(8,y,240,23,i==launcher_language_cursor?nds_theme_selected():nds_theme_card());launcher_text(nds_language_choice_name(i),18,y+3,white,240);}
 launcher_text(nds_ui("上下选择 · A 确定 · B 返回"),7,169,accent,250);
 }
 else if(launcher_about){
 launcher_text(nds_ui("PyMO：chen_xin_ming"),10,47,white,248);launcher_text(nds_ui("CPyMO：Seng Jik 及贡献者"),10,75,muted,248);launcher_text(nds_ui("游戏与汉化署名照原样保留"),10,103,muted,248);launcher_text(nds_ui("新游戏请先使用转换助手"),10,131,muted,248);
 launcher_text(nds_ui("按 B 返回游戏列表"),10,169,accent,248);}

 else {unsigned first=launcher_scroll_first;
 for(unsigned row=0;row<4&&first+row<launcher_count;row++){unsigned index=first+row;int y=33+row*31;LauncherGame* game=&launcher_games[index];
  launcher_rect(6,y,244,28,index==launcher_selected?nds_theme_selected():nds_theme_card());
  /* C scheme: a restrained two-pixel selection rail, including default. */
  if(index==launcher_selected)launcher_rect(6,y,2,28,nds_theme_rail());
  launcher_game_icon(game,10,y+2,false);
 launcher_list_title(game,40,y+6,game->status==1?white:muted);}
 launcher_scrollbar_draw();
 /* Preserve the original 28px game cards, touch hitboxes and 18px hint row spacing.
    Move both hint lines 4px up; do not redesign the UI. */
  launcher_text(message&&*message?nds_ui(message):nds_ui("A / START 启动 · 轻触两次启动"),7,156,accent,250);
  launcher_text(nds_ui("上下选择 · B 关于 · X 语言 · Y 主题"),7,174,muted,250);}
 launcher_publish(launcher_sub);launcher_state[3]=launcher_selected;frontend_state[1]=launcher_about;
 if(splash_state[1]){splash_state[1]=0;splash_state[3]++;}
}
static void launcher_release_keys(void){do{nds_power_poll();swiWaitForVBlank();scanKeys();}while(keysHeld()&(KEY_A|KEY_B|KEY_X|KEY_Y|KEY_L|KEY_TOUCH|KEY_START|KEY_SELECT));}
static void launcher_choose(char* out,size_t size,const char* hint){
 error_ui_active=false;
 if(!splash_state[1]){launcher_video_init();launcher_show_splash();}
 nds_playing=false;nds_aux_override=false;nds_aux_refresh();lcdMainOnTop();screens_swapped=false;presentation_state[1]=0;presentation_state[2]=0;launcher_icon_path[0]=0;
 free(game_glyphs);game_glyphs=NULL;game_glyph_count=0;launcher_font_path[0]=0;free_pixel_fonts();clear_menu_images();nds_cache_enabled=false;
 nds_menu_clock_visible=false;clock_state[9]=0;
 launcher_about=false;launcher_language_menu=false;launcher_theme_menu=false;launcher_refresh_notice_until=0;launcher_scroll_dragging=false;launcher_state[1]=1;launcher_scan(hint);launcher_draw(NULL);launcher_video_show();launcher_release_keys();
 while(1){nds_power_poll();swiWaitForVBlank();scanKeys();int down=keysDown(),held=keysHeld();bool redraw=false,start=false;
 if(!(held&KEY_TOUCH))launcher_scroll_dragging=false;
 if(nds_clock_sample()&&!launcher_about&&!launcher_language_menu&&!launcher_theme_menu)launcher_library_clock(launcher_sub,true);
 if(down&KEY_X){launcher_language_menu=!launcher_language_menu;launcher_theme_menu=false;launcher_about=false;launcher_language_cursor=nds_language_choice_get();launcher_draw(NULL);continue;}
 if(down&KEY_Y){launcher_theme_menu=!launcher_theme_menu;launcher_language_menu=false;launcher_about=false;launcher_theme_cursor=nds_theme_get();launcher_draw(NULL);continue;}
 if(down&KEY_B){if(launcher_language_menu)launcher_language_menu=false;else if(launcher_theme_menu)launcher_theme_menu=false;else launcher_about=!launcher_about;launcher_draw(NULL);continue;}
 if(launcher_theme_menu){
  if(down&KEY_UP){
   if(launcher_theme_cursor>=2&&launcher_theme_cursor<=5)launcher_theme_cursor=(launcher_theme_cursor-2)/2;
   else if(launcher_theme_cursor>=6)launcher_theme_cursor-=4;
  }
  if(down&KEY_DOWN){
   if(launcher_theme_cursor<2)launcher_theme_cursor=launcher_theme_cursor?4:2;
   else if(launcher_theme_cursor<14)launcher_theme_cursor+=4;
  }
  if(down&KEY_LEFT){
   if(launcher_theme_cursor==1)launcher_theme_cursor=0;
   else if(launcher_theme_cursor>=3&&(launcher_theme_cursor-2)%4)launcher_theme_cursor--;
  }
  if(down&KEY_RIGHT){
   if(launcher_theme_cursor==0)launcher_theme_cursor=1;
   else if(launcher_theme_cursor>=2&&(launcher_theme_cursor-2)%4!=3)launcher_theme_cursor++;
  }
  if(down&KEY_TOUCH){
   touchPosition t;touchRead(&t);
   if(t.py>=33&&t.py<55){if(t.px>=8&&t.px<123)launcher_theme_cursor=0;else if(t.px>=131&&t.px<248)launcher_theme_cursor=1;}
   else if(t.py>=60&&t.py<153&&t.px>=8&&t.px<248){
    unsigned col=(t.px-8)/61,row=(t.py-60)/24;
    if(col<4&&row<4)launcher_theme_cursor=2+row*4+col;
   }
  }
  if(down&KEY_A){
   const char* msg=nds_theme_set(launcher_theme_cursor)?"主题已保存":"设置保存失败";
   launcher_theme_menu=false;launcher_draw(msg);continue;
  }
  launcher_draw(NULL);continue;
 }
 if(launcher_language_menu){
  if(!(down&(KEY_UP|KEY_DOWN|KEY_A)))continue;
  if(down&KEY_UP)launcher_language_cursor=(launcher_language_cursor+NDS_LANG_COUNT-1)%NDS_LANG_COUNT;
  if(down&KEY_DOWN)launcher_language_cursor=(launcher_language_cursor+1)%NDS_LANG_COUNT;
  const char* notice=NULL;if(down&KEY_A){notice=nds_language_choice_set(launcher_language_cursor)?"语言设置已保存":"设置保存失败";nds_language_take_refresh();launcher_language_menu=false;}
  launcher_draw(notice);continue;
 }
 if(launcher_about)continue;
 if(down&KEY_SELECT){launcher_font_path[0]=0;launcher_scan(hint);launcher_refresh_notice_until=tickGetCount()+2*TICK_FREQ;redraw=true;}
 if(launcher_count){if(down&KEY_UP){launcher_selected=(launcher_selected+launcher_count-1)%launcher_count;redraw=true;}if(down&KEY_DOWN){launcher_selected=(launcher_selected+1)%launcher_count;redraw=true;}
 if(down&KEY_LEFT){launcher_selected=launcher_selected>=4?launcher_selected-4:0;redraw=true;}if(down&KEY_RIGHT){launcher_selected+=4;if(launcher_selected>=launcher_count)launcher_selected=launcher_count-1;redraw=true;}
 if(down&KEY_TOUCH){touchPosition t;touchRead(&t);
  if(launcher_scrollbar_begin_touch(t.px,t.py)){redraw=true;}
  else if(t.px>=6&&t.px<(launcher_count>4?LAUNCHER_SCROLL_TOUCH_X:250)&&t.py>=33&&t.py<157){
   unsigned index=launcher_scroll_first+(t.py-33)/31;
   if(index<launcher_count){start=index==launcher_selected;launcher_selected=index;redraw=true;}
  }
 }
 if(launcher_scroll_dragging&&(held&KEY_TOUCH)){
  touchPosition t;touchRead(&t);
  if(launcher_scrollbar_move_touch(t.py))redraw=true;
 }
 if(down&(KEY_A|KEY_START))start=true;
 if(start){LauncherGame* game=&launcher_games[launcher_selected];if(game->status==1){launcher_remember();snprintf(out,size,"%s",game->path);launcher_release_keys();launcher_state[1]=0;launcher_state[4]++;return;}launcher_draw("请在电脑上先转换或修复缓存");continue;}}
 if(launcher_refresh_notice_until&&tickGetCount()>=launcher_refresh_notice_until){launcher_refresh_notice_until=0;redraw=true;}
 if(redraw)launcher_draw(launcher_refresh_notice_until?"游戏列表已刷新":NULL);
 }
}
