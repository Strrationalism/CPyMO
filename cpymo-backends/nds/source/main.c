#include "release-version.h"
#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/iosupport.h>
#include "cpymo_engine.h"
#include "cpymo_save.h"
#include "cpymo_config_ui.h"
#include "cpymo_list_ui.h"
#include "cpymo_rmenu.h"
#include "cpymo_ui.h"
#include "ui-language-nds.h"
#include "cpymo_backend_masktrans.h"
#include "font.h"
#include "cache-nds.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
/* Keep audio supplied while the original JPEG/PNG decoder reads the card.
   This wrapper leaves upstream CPyMO and stb sources unchanged. */
extern stbi_uc* __real_stbi_load_from_callbacks(const stbi_io_callbacks*,void*,int*,int*,int*,int);
extern void nds_audio_update(void);
extern void nds_audio_pump_if_due(void);
typedef struct {const stbi_io_callbacks* cb;void* user;u64 last;} PumpIO;
static int pump_read(void* user,char* data,int size){PumpIO* io=user;int n=io->cb->read(io->user,data,size);u64 now=tickGetCount();if(now-io->last>TICK_FREQ/100){nds_audio_update();io->last=tickGetCount();}return n;}
static void pump_skip(void* user,int n){PumpIO* io=user;io->cb->skip(io->user,n);}
static int pump_eof(void* user){PumpIO* io=user;return io->cb->eof(io->user);}
stbi_uc* __wrap_stbi_load_from_callbacks(const stbi_io_callbacks* cb,void* user,int* w,int* h,int* channels,int requested){PumpIO io={cb,user,tickGetCount()};stbi_io_callbacks pump={pump_read,pump_skip,pump_eof};return __real_stbi_load_from_callbacks(&pump,&io,w,h,channels,requested);}

static cpymo_engine engine;
/* UI requests only set this latch; the main game loop owns cleanup. */
static bool nds_library_exit_requested,nds_title_restart_pending;
void nds_request_exit_to_library(void){nds_library_exit_requested=true;}
/* Restart callback runs during UI update; only set a latch here. */
void nds_request_title_guide_reset(void){nds_title_restart_pending=true;}

static u16 framebuffer[256*192] __attribute__((aligned(32)));
static u16 *vram;
/* Cache the exact immutable base-layer command sequence, never dialogue/UI/effects. */
typedef struct {float dx,dy,dw,dh,alpha;cpymo_backend_image image;int sx,sy,sw,sh,type;} SceneCommand;
static SceneCommand scene_commands[64],scene_previous[64];
static unsigned scene_count,scene_previous_count;static bool scene_collect,scene_overflow,scene_valid;
static float scene_view[3];static u16* scene_pixels;static u16* scene_verify_pixels;
volatile unsigned scene_state[8]={0x53435238};
void nds_scene_release(void){scene_valid=false;if(scene_pixels){free(scene_pixels);scene_pixels=NULL;scene_state[4]++;}}

static bool screens_swapped;
#include "save-dir-nds.h"
#include "preferences-nds.h"
/* magic, swapped, toast active/shown/expired/age ms, icon loaded, metadata lines, touch samples */
volatile unsigned presentation_state[9]={0x4e505232};
#include "dual-screen-nds.h"
#include "dual-input-nds.h"
static nds_touch_role_tracker nds_touch_role_state;
/* Runs BEFORE cpymo_engine_update snapshots input as prev_input.
 * Do not allow a pen already pressing story-A to become a new click
 * when CPyMO opens a touch-controlled choice or menu (or vice versa). */
static void nds_touch_prepare_input(unsigned held){
 unsigned coords=nds_touch_uses_coordinates(nds_display_mode,
   nds_dual_active(),cpymo_ui_enabled(&engine),nds_dual_has_choice());
 unsigned boundary=nds_touch_role_step(&nds_touch_role_state,
                                       coords,!!(held&KEY_TOUCH));
 if(!boundary)return;
 cpymo_input_ignore_next_mouse_button_event(&engine);
 engine.input.ok=!!(held&KEY_A);
 engine.input.mouse_button=false;
 engine.input.mouse_position_useable=false;
 if(boundary==2)nds_touch_blocked=true;
}
volatile unsigned engine_state[46]={0x4350454e};
/* magic, image loads, total/max load us, draws, total/max draw us, loop pixels, omitted pixels */
volatile unsigned perf_state[9]={0x4e505246};
volatile unsigned draw_trace[4+64*8]={0x44545238};
static unsigned text_us,fill_us;
/* magic, auto/hide/X/skip flags, current script line; bounded test telemetry */
volatile unsigned control_state[3]={0x4e43544c};
bool nds_cache_enabled;
static bool loaded_image;
extern void nds_audio_update(void);
volatile unsigned vblank_count;
static void on_vblank(void){++vblank_count;}
static float display_scale(void){
 int w=engine.gameconfig.imagesize_w,h=engine.gameconfig.imagesize_h;
 if(w<=0||h<=0)return 1;
 float sx=256.f/w,sy=192.f/h;return sx<sy?sx:sy;
}
static unsigned text_native_pixels(void);
int nds_font_choices_available(void);
static void enforce_text_readability(void){if(engine.gameconfig.imagesize_h>0){int min_font=(int)(text_native_pixels()*240.f/(engine.gameconfig.imagesize_h*.8f*display_scale())+.999f);if(nds_font_choices_available()||engine.gameconfig.fontsize<min_font)engine.gameconfig.fontsize=min_font;}}
static float display_x(void){return (256-engine.gameconfig.imagesize_w*display_scale())*.5f;}
static float display_y(void){return (192-engine.gameconfig.imagesize_h*display_scale())*.5f;}
/* Locate the actual menu panel, not the screen corner. */
static bool nds_clock_collect_menu,nds_clock_menu_found;
static int nds_clock_menu_x,nds_clock_menu_y;
static void nds_clock_menu_rect(float x,float y,enum cpymo_backend_image_draw_type type){
 if(!nds_clock_collect_menu||nds_clock_menu_found||type!=cpymo_backend_image_draw_type_ui_element_bg)return;
 nds_clock_menu_x=(int)(x*display_scale()+display_x());
 nds_clock_menu_y=(int)(y*display_scale()+display_y());
 if(nds_clock_menu_x<0)nds_clock_menu_x=0;
 if(nds_clock_menu_y<0)nds_clock_menu_y=0;
 nds_clock_menu_found=true;
}
/* bit0=opaque spans, bit1=mask x map; private reference switch. */
volatile unsigned render_policy[8]={0x52505231,3};
static unsigned render_bits=3;
typedef struct {int w,h,pw,ph;u16 *px;unsigned char *mask;unsigned refs;u16* bounds;u16* opaque;bool opaque_checked;} Image;

volatile unsigned load_trace[4+64*14]={0x4c545231};
static unsigned image_buffer_policy;static bool native_row_copy=false;static bool renderer_fast=true;static bool image_sequential;static unsigned cache_layout_policy;
volatile unsigned load_state[32]={0x4c445046};
static void load_phase(unsigned phase,u64 start){unsigned us=(tickGetCount()-start)*1000000/TICK_FREQ;load_state[phase]+=us;if(us>load_state[phase+12])load_state[phase+12]=us;}

#include "npl4-reader.h"
#include "nps-reader.h"
#include "opaque-span-r12.h"
static void prepare_bounds(Image* im){
 if(!im->mask)return;
 im->bounds=malloc(im->ph*2*sizeof(u16));if(!im->bounds)return;

 for(int y=0;y<im->ph;y++){if(!(y&7))nds_audio_pump_if_due();int l=0,r=im->pw;const unsigned char* m=im->mask+y*im->pw;
  while(l<r&&!m[l])l++;while(r>l&&!m[r-1])r--;
  im->bounds[y*2]=l;im->bounds[y*2+1]=r;

 }
}

/* Defer optional metadata to the first partial-alpha draw, away from the
   scene's file-loading update. Reserve its potential size in the image cache. */
static void prepare_opaque(Image* im){
 im->opaque_checked=true;
 if(!im->mask||!im->bounds)return;
 extern void* __real_malloc(size_t);im->opaque=__real_malloc(im->ph*2*sizeof(u16));
 if(!im->opaque){render_policy[6]++;return;}render_policy[2]++;
 for(int y=0;y<im->ph;y++){if(!(y&7))nds_audio_pump_if_due();opaque_span(im->mask+y*im->pw,im->bounds[y*2],im->bounds[y*2+1],im->opaque+y*2);}
}

/* Bounded recent character-image cache: owns one reference, never changes pixels. */
#define RECENT_LIMIT (384u*1024u)
#define RECENT_LIMIT_TWL (2048u*1024u)
/* Keep the original DS budget except on a genuine TWL runtime with >=8 MiB
 * of heap address space. No direct clock, SCFG, or heap-bound changes. */
static unsigned nds_recent_image_budget(void){
 if(!isDSiMode())return RECENT_LIMIT;
 const uintptr_t lo=(uintptr_t)getHeapStart(),hi=(uintptr_t)getHeapLimit();
 return hi>lo && hi-lo>=(8u*1024u*1024u)?RECENT_LIMIT_TWL:RECENT_LIMIT;
}
typedef struct {char path[512];Image* image;unsigned bytes,age;} RecentImage;
/* NTR keeps the original six-slot policy. TWL's 2 MiB budget can retain
 * more recently used sprites; more memory is NOT reserved for sprite pixels
 * unless those assets are actually loaded. */
#define RECENT_SLOTS_DS 6u
#define RECENT_SLOTS_TWL 12u
static RecentImage recent_images[RECENT_SLOTS_TWL];
static unsigned recent_bytes,recent_age,recent_limit=RECENT_LIMIT;
/* Diagnostic-only counters: magic, background hits, misses, stores, skips.
 * Background reuse is TWL-only, sharing the existing 2 MiB LRU with sprites. */
volatile unsigned background_cache_state[5]={0x42474331,0,0,0,0};
static bool nds_bg_cache_enabled;
static unsigned recent_capacity(void){
 return recent_limit>=RECENT_LIMIT_TWL?RECENT_SLOTS_TWL:RECENT_SLOTS_DS;
}
static void recent_remove(unsigned i){if(recent_images[i].image){recent_bytes-=recent_images[i].bytes;cpymo_backend_image_free(recent_images[i].image);}memset(&recent_images[i],0,sizeof recent_images[i]);}
static void recent_clear(void){for(unsigned i=0;i<RECENT_SLOTS_TWL;i++)recent_remove(i);recent_age=0;}
static Image* recent_find(const char* path){for(unsigned i=0;i<recent_capacity();i++)if(recent_images[i].image&&!strcmp(recent_images[i].path,path)){recent_images[i].age=++recent_age;recent_images[i].image->refs++;load_state[9]++;return recent_images[i].image;}load_state[10]++;return NULL;}
static bool recent_store(const char* path,Image* im){
 unsigned cost=sizeof *im+im->pw*im->ph*(im->mask?3:2)+(im->bounds?im->ph*4:0)+(((render_bits&1)&&im->mask&&im->bounds)?im->ph*4:0);
 if(cost>recent_limit)return false;
 int slot=-1;
 for(;;){slot=-1;for(unsigned i=0;i<recent_capacity();i++)if(!recent_images[i].image){slot=i;break;}
  if(slot>=0&&recent_bytes+cost<=recent_limit)break;
  unsigned oldest=0;for(unsigned i=1;i<recent_capacity();i++)if(recent_images[i].image&&(!recent_images[oldest].image||recent_images[i].age<recent_images[oldest].age))oldest=i;
  recent_remove(oldest);load_state[11]++;
 }
 RecentImage* r=&recent_images[slot];snprintf(r->path,sizeof r->path,"%s",path);r->image=im;r->bytes=cost;r->age=++recent_age;im->refs++;recent_bytes+=cost;
 if(recent_bytes>load_state[30])load_state[30]=recent_bytes;
 return true;
}
static Image* menu_images[2];
static char menu_image_dir[256];
void nds_cache_hash_path(char* out,size_t size,const char* dir,const char* kind,const char* hex,const char* ext){
 unsigned mask=(!strcmp(kind,"bgm")||!strcmp(kind,"se")||!strcmp(kind,"voice"))?2:1;
 if(cache_layout_policy&mask)snprintf(out,size,"%s/nds-cache/%s/%.2s/%s%s",dir,kind,hex,hex,ext);
 else snprintf(out,size,"%s/nds-cache/%s/%s%s",dir,kind,hex,ext);
}
void nds_cache_path(char* out,size_t size,const char* dir,const char* kind,cpymo_str name,const char* ext){
 u64 hash=14695981039346656037ULL;for(size_t i=0;i<name.len;i++){unsigned char c=name.begin[i];if(c>='A'&&c<='Z')c+='a'-'A';hash^=c;hash*=1099511628211ULL;}
 char hex[17];snprintf(hex,sizeof hex,"%016llx",(unsigned long long)hash);nds_cache_hash_path(out,size,dir,kind,hex,ext);
}
static error_t cached_image(cpymo_backend_image* out,int* w,int* h,cpymo_str name,const cpymo_assetloader* loader,const char* kind){
 u64 perf_start=tickGetCount(),phase_start=perf_start;unsigned old_phase[8];for(unsigned i=1;i<8;i++)old_phase[i]=load_state[i];unsigned old_decode=compressed_state[3],old_check=compressed_state[5];
 char path[512];nds_cache_path(path,sizeof path,loader->gamedir,kind,name,".npi");
 const bool is_bg=!strcmp(kind,"bg");
 /* Sprite reuse remains unchanged on NTR; only actual TWL heap profiles
  * reuse backgrounds. Both resource kinds share the same bounded LRU. */
 const bool reuse=!strcmp(kind,"chara")||(is_bg&&nds_bg_cache_enabled);
 if(reuse){
  Image* hit=recent_find(path);
  if(is_bg)background_cache_state[hit?1:2]++;
  if(hit){*out=hit;*w=hit->w;*h=hit->h;loaded_image=true;engine_state[40]++;nds_audio_pump_if_due();return CPYMO_ERR_SUCC;}
 }
 FILE* f=fopen(path,"rb");if(!f)return CPYMO_ERR_CAN_NOT_OPEN_FILE;
 if(image_buffer_policy&&setvbuf(f,NULL,image_buffer_policy==1?_IONBF:_IOFBF,image_buffer_policy==2?8192:4096)&&setvbuf(f,NULL,_IONBF,0)){fclose(f);return CPYMO_ERR_OUT_OF_MEM;}

 load_phase(1,phase_start);phase_start=tickGetCount();
 struct {char magic[4];u16 w,h,pw,ph;u32 flags;} header;
 if(fread(&header,1,sizeof header,f)!=sizeof header||(memcmp(header.magic,"NPI1",4)&&memcmp(header.magic,"NPA4",4)&&memcmp(header.magic,"NPS1",4)&&memcmp(header.magic,"NPS2",4))||!header.w||!header.h||!header.pw||!header.ph||header.pw>2048||header.ph>2048||(unsigned)header.pw*header.ph>262144||header.flags>1){fclose(f);return CPYMO_ERR_BAD_FILE_FORMAT;}
 unsigned count=(unsigned)header.pw*header.ph;
 bool packed=!memcmp(header.magic,"NPA4",4),sparse=!memcmp(header.magic,"NPS1",4)||!memcmp(header.magic,"NPS2",4);
 if(sparse&&header.flags!=1){fclose(f);return CPYMO_ERR_BAD_FILE_FORMAT;}
 if(!packed&&!sparse&&!image_sequential&&(fseek(f,0,SEEK_END)||ftell(f)!=(long)(sizeof header+count*(header.flags?3:2))||fseek(f,sizeof header,SEEK_SET))){fclose(f);return CPYMO_ERR_BAD_FILE_FORMAT;}
 load_phase(2,phase_start);phase_start=tickGetCount();
 Image* image=calloc(1,sizeof *image);if(!image){fclose(f);if(recent_bytes){recent_clear();return cached_image(out,w,h,name,loader,kind);}return CPYMO_ERR_OUT_OF_MEM;}
 image->refs=1;image->w=header.w;image->h=header.h;image->pw=header.pw;image->ph=header.ph;image->px=malloc(count*2);if(header.flags)image->mask=malloc(count);
 if(!image->px||(header.flags&&!image->mask)){fclose(f);free(image->px);free(image->mask);free(image);if(recent_bytes){recent_clear();return cached_image(out,w,h,name,loader,kind);}return CPYMO_ERR_OUT_OF_MEM;}
 load_phase(3,phase_start);phase_start=tickGetCount();
 unsigned bytes=count*2,done=0;bool ok=true;
 if(sparse){int result=nps_file(f,image,!memcmp(header.magic,"NPS2",4));if(result<0){fclose(f);free(image->px);free(image->mask);free(image);if(recent_bytes){recent_clear();return cached_image(out,w,h,name,loader,kind);}return CPYMO_ERR_OUT_OF_MEM;}ok=result>0;phase_start=tickGetCount();}
 else if(packed){unsigned char* input=malloc(8192);if(!input){fclose(f);free(image->px);free(image->mask);free(image);if(recent_bytes){recent_clear();return cached_image(out,w,h,name,loader,kind);}return CPYMO_ERR_OUT_OF_MEM;}
  ok=npl_plane(f,(unsigned char*)image->px,bytes,input);
  if(ok&&image->mask)ok=npl_plane(f,image->mask,count,input);
  if(ok)ok=fgetc(f)==EOF&&!ferror(f);free(input);phase_start=tickGetCount();
 }else{
 while(done<bytes){unsigned chunk=bytes-done;if(chunk>8192)chunk=8192;if(fread((unsigned char*)image->px+done,1,chunk,f)!=chunk){ok=false;break;}done+=chunk;load_phase(4,phase_start);phase_start=tickGetCount();nds_audio_update();load_phase(5,phase_start);phase_start=tickGetCount();}
 if(ok&&image->mask){done=0;while(done<count){unsigned chunk=count-done;if(chunk>8192)chunk=8192;if(fread(image->mask+done,1,chunk,f)!=chunk){ok=false;break;}done+=chunk;load_phase(4,phase_start);phase_start=tickGetCount();nds_audio_update();load_phase(5,phase_start);phase_start=tickGetCount();}}
 }
 if(ok&&!packed&&!sparse&&image_sequential)ok=fgetc(f)==EOF&&!ferror(f);
 load_phase(4,phase_start);phase_start=tickGetCount();fclose(f);load_phase(6,phase_start);if(!ok){free(image->px);free(image->mask);free(image);return CPYMO_ERR_BAD_FILE_FORMAT;}
 phase_start=tickGetCount();prepare_bounds(image);load_phase(7,phase_start);load_state[8]++;
 if(reuse){
  bool stored=recent_store(path,image);
  if(is_bg)background_cache_state[stored?3:4]++;
 }
 unsigned perf_us=(tickGetCount()-perf_start)*1000000/TICK_FREQ;perf_state[1]++;perf_state[2]+=perf_us;if(perf_us>perf_state[3])perf_state[3]=perf_us;
 unsigned seq=load_trace[1]++,j=4+(seq%64)*14;load_trace[j]=seq;load_trace[j+1]=perf_us;for(unsigned i=1;i<8;i++)load_trace[j+1+i]=load_state[i]-old_phase[i];load_trace[j+9]=!strcmp(kind,"bg")?0:!strcmp(kind,"chara")?1:2;load_trace[j+10]=count;load_trace[j+11]=engine_state[14];load_trace[j+12]=compressed_state[3]-old_decode;load_trace[j+13]=compressed_state[5]-old_check;
 *out=image;*w=image->w;*h=image->h;loaded_image=true;engine_state[40]++;return CPYMO_ERR_SUCC;
}
extern error_t __real_cpymo_assetloader_load_bg_image(cpymo_backend_image*,int*,int*,cpymo_str,const cpymo_assetloader*);
extern error_t __real_cpymo_assetloader_load_chara_image(cpymo_backend_image*,int*,int*,cpymo_str,const cpymo_assetloader*);
extern error_t __real_cpymo_assetloader_load_system_image(cpymo_backend_image*,int*,int*,cpymo_str,const cpymo_assetloader*,bool);
error_t __wrap_cpymo_assetloader_load_bg_image(cpymo_backend_image* out,int* w,int* h,cpymo_str name,const cpymo_assetloader* l){if(nds_cache_enabled)return cached_image(out,w,h,name,l,"bg");return __real_cpymo_assetloader_load_bg_image(out,w,h,name,l);}
error_t __wrap_cpymo_assetloader_load_chara_image(cpymo_backend_image* out,int* w,int* h,cpymo_str name,const cpymo_assetloader* l){if(nds_cache_enabled)return cached_image(out,w,h,name,l,"chara");return __real_cpymo_assetloader_load_chara_image(out,w,h,name,l);}
static void clear_menu_images(void){for(unsigned i=0;i<2;i++){if(menu_images[i])cpymo_backend_image_free(menu_images[i]);menu_images[i]=NULL;}menu_image_dir[0]=0;}
error_t __wrap_cpymo_assetloader_load_system_image(cpymo_backend_image* out,int* w,int* h,cpymo_str name,const cpymo_assetloader* l,bool mask){
 if(!nds_cache_enabled)return __real_cpymo_assetloader_load_system_image(out,w,h,name,l,mask);
 int slot=mask&&name.len==4&&!memcmp(name.begin,"menu",4)?0:mask&&name.len==13&&!memcmp(name.begin,"sel_highlight",13)?1:-1;
 if(slot<0)return cached_image(out,w,h,name,l,mask?"system1":"system0");
 if(strcmp(menu_image_dir,l->gamedir)){clear_menu_images();snprintf(menu_image_dir,sizeof menu_image_dir,"%s",l->gamedir);}
 if(!menu_images[slot]){error_t err=cached_image(out,w,h,name,l,"system1");if(err)return err;menu_images[slot]=*out;}
 Image* image=menu_images[slot];image->refs++;*out=image;*w=image->w;*h=image->h;return CPYMO_ERR_SUCC;
}
static void warm_menu_images(void){for(unsigned i=0;i<2;i++){cpymo_backend_image image=NULL;int w,h;error_t err=__wrap_cpymo_assetloader_load_system_image(&image,&w,&h,cpymo_str_pure(i?"sel_highlight":"menu"),&engine.assetloader,true);if(!err)cpymo_backend_image_free(image);nds_audio_update();}}
static u16 color(cpymo_color c){return RGB15(c.r>>3,c.g>>3,c.b>>3)|BIT(15);}
static void pixel(int x,int y,cpymo_color c,float alpha){
 if(x<0||x>=256||y<0||y>=192||alpha<=0)return;
 unsigned idx=y*256+x;if(alpha>=.99f){framebuffer[idx]=color(c);return;}
 u16 old=framebuffer[idx];int a=(int)(alpha*32);if(a>32)a=32;
 int r=((old&31)*(32-a)+(c.r>>3)*a)>>5;
 int g=(((old>>5)&31)*(32-a)+(c.g>>3)*a)>>5;
 int b=(((old>>10)&31)*(32-a)+(c.b>>3)*a)>>5;
 framebuffer[idx]=RGB15(r,g,b)|BIT(15);
}
/* Integer area filter: load-time only, before RGB555 quantization.
   Alpha images accumulate premultiplied channels to avoid dark fringes. */
static void shrink_pixels(const unsigned char* src,int w,int h,unsigned char* dst,int nw,int nh,int channels){
 for(int y=0;y<nh;y++){
  if(!(y&7))nds_audio_update();
  if(channels==3&&w*4==nw*5&&h*4==nh*5){
   int sy=y*5/4,wy=4-(y&3);unsigned char* out=dst+y*nw*3;
   for(int x=0;x<nw;x++,out+=3){int sx=x*5/4,wx=4-(x&3);const unsigned char* p=src+(sy*w+sx)*3;
    for(int c=0;c<3;c++)out[c]=(p[c]*wx*wy+p[c+3]*(5-wx)*wy+p[c+w*3]*wx*(5-wy)+p[c+w*3+3]*(5-wx)*(5-wy)+12)/25;
   }
   continue;
  }
  for(int x=0;x<nw;x++){
  int x0=x*w*256/nw,x1=(x+1)*w*256/nw,y0=y*h*256/nh,y1=(y+1)*h*256/nh;
  unsigned sums[4]={0},area=(x1-x0)*(y1-y0);
  for(int sy=y0>>8;sy<(y1+255)>>8;sy++)for(int sx=x0>>8;sx<(x1+255)>>8;sx++){
   int lo=sx*256>x0?sx*256:x0,hi=(sx+1)*256<x1?(sx+1)*256:x1,wx=hi-lo;
   lo=sy*256>y0?sy*256:y0;hi=(sy+1)*256<y1?(sy+1)*256:y1;
   unsigned weight=wx*(hi-lo);const unsigned char* p=src+(sy*w+sx)*channels;
   for(int c=0;c<channels;c++){unsigned v=p[c];if(channels==4&&c<3)v=(v*p[3]+127)/255;sums[c]+=v*weight;}
  }
  unsigned char* out=dst+(y*nw+x)*channels;
  for(int c=0;c<channels;c++){if(channels==4&&c<3)out[c]=sums[3]?(unsigned)(((u64)sums[c]*255+sums[3]/2)/sums[3]):0;else out[c]=(sums[c]+area/2)/area;}
  }
 }
}
error_t cpymo_backend_image_load(cpymo_backend_image *out,void *px,int w,int h,enum cpymo_backend_image_format f){
 engine_state[43]++;
 loaded_image=true;
 Image *im=calloc(1,sizeof *im);if(!im)return CPYMO_ERR_OUT_OF_MEM;
 im->refs=1;im->w=w;im->h=h;
 float scale=display_scale();if(scale>1)scale=1;
 im->pw=(int)(w*scale+.5f);im->ph=(int)(h*scale+.5f);
 if(im->pw<1)im->pw=1;if(im->ph<1)im->ph=1;
 int channels=f==cpymo_backend_image_format_rgb?3:4;
 unsigned char *filtered=NULL;
 if(im->pw!=w||im->ph!=h){
  filtered=malloc(im->pw*im->ph*channels);
  if(!filtered){free(im);return CPYMO_ERR_OUT_OF_MEM;}
  shrink_pixels(px,w,h,filtered,im->pw,im->ph,channels);
 }
 im->px=malloc(im->pw*im->ph*2);
 if(channels==4)im->mask=malloc(im->pw*im->ph);
 if(!im->px||(channels==4&&!im->mask)){free(filtered);free(im->px);free(im->mask);free(im);return CPYMO_ERR_OUT_OF_MEM;}
 unsigned char* p=filtered?filtered:px;
 for(int i=0;i<im->pw*im->ph;i++,p+=channels){im->px[i]=RGB15(p[0]>>3,p[1]>>3,p[2]>>3)|BIT(15);if(im->mask)im->mask[i]=p[3];}
 free(filtered);
 prepare_bounds(im);free(px);*out=im;return CPYMO_ERR_SUCC;
}
error_t cpymo_backend_image_load_with_mask(cpymo_backend_image *out,void *px,void *mask,int w,int h,int mw,int mh){
 unsigned char* p=px;unsigned char* m=mask;
 for(int y=0;y<h;y++)for(int x=0;x<w;x++)p[(y*w+x)*4+3]=m[(y*mh/h)*mw+x*mw/w];
 error_t err=cpymo_backend_image_load(out,px,w,h,cpymo_backend_image_format_rgba);
 if(!err)free(mask);
 return err;
}
void cpymo_backend_image_free(cpymo_backend_image im){if(im){nds_scene_release();Image* image=im;if(--image->refs)return;free(image->opaque);free(image->bounds);free(image->px);free(image->mask);free(image);}}
/* Independent red/blue lanes are ten bits apart: sums stay <=31*32.
   Exact RGB555 blend, using two multiplies for both lanes instead of four. */
static u16 blend(u16 old,u16 col,int a){if(a>=32)return col;if(a<=0)return old;unsigned rb=(((old&0x7c1f)*(32-a)+(col&0x7c1f)*a+0x4010u)>>5)&0x7c1f;unsigned g=(((old&0x03e0)*(32-a)+(col&0x03e0)*a+0x0200u)>>5)&0x03e0;return rb|g|BIT(15);}
static void image_draw_reference(float dx,float dy,float dw,float dh,cpymo_backend_image image,int sx,int sy,int sw,int sh,float alpha,enum cpymo_backend_image_draw_type type){
 if(scene_collect){
  if(scene_count>=64){scene_overflow=true;return;}
  SceneCommand* c=&scene_commands[scene_count++];memset(c,0,sizeof *c);
  c->dx=dx;c->dy=dy;c->dw=dw;c->dh=dh;c->alpha=alpha;c->image=image;c->sx=sx;c->sy=sy;c->sw=sw;c->sh=sh;c->type=type;return;
 }
 u64 perf_start=tickGetCount();
 (void)type;Image* im=image;if(!im||dw<=0||dh<=0)return;
 float scale=display_scale();dx=dx*scale+display_x();dy=dy*scale+display_y();dw*=scale;dh*=scale;
 int a=(int)(alpha*32);if(a<=0)return;if(a>32)a=32;
 if((render_bits&1)&&im->mask&&!im->opaque_checked&&(a<32||native_row_copy))prepare_opaque(im);
 int left=(int)dx,top=(int)dy,right=(int)(dx+dw),bottom=(int)(dy+dh);
 int x0=left<0?0:left,y0=top<0?0:top,x1=right>256?256:right,y1=bottom>192?192:bottom;
 if(x1<=x0||y1<=y0)return;
 int startX=(int)(sx*(float)im->pw/im->w*65536.f),startY=(int)(sy*(float)im->ph/im->h*65536.f);
 unsigned char opacity[256];if(im->mask)for(unsigned i=0;i<256;i++)opacity[i]=(a*i+127)/255;
 int stepX=(int)(sw*(float)im->pw/im->w*65536.f/dw+.5f),stepY=(int)(sh*(float)im->ph/im->h*65536.f/dh+.5f);
 int columns[256];for(int x=x0;x<x1;x++){int ix=(startX+(x-left)*stepX)>>16;columns[x]=ix>=0&&ix<im->pw?ix:-1;}
 for(int y=y0;y<y1;y++){
  if(!(y&15))nds_audio_pump_if_due();
  int iy=(startY+(y-top)*stepY)>>16;if(iy<0||iy>=im->ph)continue;
  int sourceX=(startX>>16)+x0-left;
  if(!im->mask && a==32 && stepX==65536 && sourceX>=0 && sourceX+x1-x0<=im->pw){memcpy(framebuffer+y*256+x0,im->px+iy*im->pw+sourceX,(x1-x0)*2);continue;}
  int first=x0,last=x1;
  if(im->bounds&&stepX>0){int l=im->bounds[iy*2],r=im->bounds[iy*2+1];
   if(l==r){perf_state[8]+=x1-x0;continue;}
   while(first<last&&(columns[first]<l||columns[first]>=r))first++;
   while(last>first&&(columns[last-1]<l||columns[last-1]>=r))last--;
  }
  perf_state[8]+=(x1-x0)-(last-first);perf_state[7]+=last-first;
  if((render_bits&1)&&im->opaque&&stepX>0){
   int ol=im->opaque[iy*2],orr=im->opaque[iy*2+1],f=first,l=last;
   if(orr>ol){
    while(f<l&&(columns[f]<ol||columns[f]>=orr))f++;
    while(l>f&&(columns[l-1]<ol||columns[l-1]>=orr))l--;
   }else f=l;
   if(f<l){
    render_policy[3]++;render_policy[4]+=l-f;
    for(int x=first;x<f;x++){int ix=columns[x];if(ix<0)continue;int idx=iy*im->pw+ix;int aa=opacity[im->mask[idx]];if(!aa)continue;if(aa==32)framebuffer[y*256+x]=im->px[idx];else framebuffer[y*256+x]=blend(framebuffer[y*256+x],im->px[idx],aa);}
    if(a==32){
     if(native_row_copy&&stepX==65536){memcpy(framebuffer+y*256+f,im->px+iy*im->pw+columns[f],(l-f)*sizeof(u16));render_policy[7]+=l-f;}
     else for(int x=f;x<l;x++)framebuffer[y*256+x]=im->px[iy*im->pw+columns[x]];
    }
    else{for(int x=f;x<l;x++)framebuffer[y*256+x]=blend(framebuffer[y*256+x],im->px[iy*im->pw+columns[x]],a);}
    for(int x=l;x<last;x++){int ix=columns[x];if(ix<0)continue;int idx=iy*im->pw+ix;int aa=opacity[im->mask[idx]];if(!aa)continue;if(aa==32)framebuffer[y*256+x]=im->px[idx];else framebuffer[y*256+x]=blend(framebuffer[y*256+x],im->px[idx],aa);}
    continue;
   }
  }
  for(int x=first;x<last;x++){int ix=columns[x];if(ix<0)continue;int idx=iy*im->pw+ix;int aa=im->mask?opacity[im->mask[idx]]:a;if(!aa)continue;if(aa==32)framebuffer[y*256+x]=im->px[idx];else framebuffer[y*256+x]=blend(framebuffer[y*256+x],im->px[idx],aa);}
 }
 unsigned perf_us=(tickGetCount()-perf_start)*1000000/TICK_FREQ;perf_state[4]++;perf_state[5]+=perf_us;if(perf_us>perf_state[6])perf_state[6]=perf_us;
}
__attribute__((optimize("O3"))) static void image_draw_fast(float dx,float dy,float dw,float dh,cpymo_backend_image image,int sx,int sy,int sw,int sh,float alpha,enum cpymo_backend_image_draw_type type){
 if(scene_collect){
  if(scene_count>=64){scene_overflow=true;return;}
  SceneCommand* c=&scene_commands[scene_count++];memset(c,0,sizeof *c);
  c->dx=dx;c->dy=dy;c->dw=dw;c->dh=dh;c->alpha=alpha;c->image=image;c->sx=sx;c->sy=sy;c->sw=sw;c->sh=sh;c->type=type;return;
 }
 u64 perf_start=tickGetCount();
 (void)type;Image* im=image;if(!im||dw<=0||dh<=0)return;
 float scale=display_scale();dx=dx*scale+display_x();dy=dy*scale+display_y();dw*=scale;dh*=scale;
 int a=(int)(alpha*32);if(a<=0)return;if(a>32)a=32;
 if((render_bits&1)&&im->mask&&!im->opaque_checked&&(a<32||native_row_copy))prepare_opaque(im);
 int left=(int)dx,top=(int)dy,right=(int)(dx+dw),bottom=(int)(dy+dh);
 int x0=left<0?0:left,y0=top<0?0:top,x1=right>256?256:right,y1=bottom>192?192:bottom;
 if(x1<=x0||y1<=y0)return;
 int startX=(int)(sx*(float)im->pw/im->w*65536.f),startY=(int)(sy*(float)im->ph/im->h*65536.f);
 unsigned char opacity[256];if(im->mask)for(unsigned i=0;i<256;i++)opacity[i]=(a*i+127)/255;
 int stepX=(int)(sw*(float)im->pw/im->w*65536.f/dw+.5f),stepY=(int)(sh*(float)im->ph/im->h*65536.f/dh+.5f);
 int columns[256];for(int x=x0;x<x1;x++){int ix=(startX+(x-left)*stepX)>>16;columns[x]=ix>=0&&ix<im->pw?ix:-1;}
 for(int y=y0;y<y1;y++){
  if(!(y&15))nds_audio_pump_if_due();
  int iy=(startY+(y-top)*stepY)>>16;if(iy<0||iy>=im->ph)continue;
  int sourceX=(startX>>16)+x0-left;
  if(!im->mask && a==32 && stepX==65536 && sourceX>=0 && sourceX+x1-x0<=im->pw){memcpy(framebuffer+y*256+x0,im->px+iy*im->pw+sourceX,(x1-x0)*2);continue;}
  int first=x0,last=x1;
  if(im->bounds&&stepX>0){int l=im->bounds[iy*2],r=im->bounds[iy*2+1];
   if(l==r){perf_state[8]+=x1-x0;continue;}
   while(first<last&&(columns[first]<l||columns[first]>=r))first++;
   while(last>first&&(columns[last-1]<l||columns[last-1]>=r))last--;
  }
  perf_state[8]+=(x1-x0)-(last-first);perf_state[7]+=last-first;
  if((render_bits&1)&&im->opaque&&stepX>0){
   int ol=im->opaque[iy*2],orr=im->opaque[iy*2+1],f=first,l=last;
   if(orr>ol){
    while(f<l&&(columns[f]<ol||columns[f]>=orr))f++;
    while(l>f&&(columns[l-1]<ol||columns[l-1]>=orr))l--;
   }else f=l;
   if(f<l){
    render_policy[3]++;render_policy[4]+=l-f;
    for(int x=first;x<f;x++){int ix=columns[x];if(ix<0)continue;int idx=iy*im->pw+ix;int aa=opacity[im->mask[idx]];if(!aa)continue;if(aa==32)framebuffer[y*256+x]=im->px[idx];else framebuffer[y*256+x]=blend(framebuffer[y*256+x],im->px[idx],aa);}
    if(a==32){
     if(native_row_copy&&stepX==65536){memcpy(framebuffer+y*256+f,im->px+iy*im->pw+columns[f],(l-f)*sizeof(u16));render_policy[7]+=l-f;}
     else for(int x=f;x<l;x++)framebuffer[y*256+x]=im->px[iy*im->pw+columns[x]];
    }
    else{for(int x=f;x<l;x++)framebuffer[y*256+x]=blend(framebuffer[y*256+x],im->px[iy*im->pw+columns[x]],a);}
    for(int x=l;x<last;x++){int ix=columns[x];if(ix<0)continue;int idx=iy*im->pw+ix;int aa=opacity[im->mask[idx]];if(!aa)continue;if(aa==32)framebuffer[y*256+x]=im->px[idx];else framebuffer[y*256+x]=blend(framebuffer[y*256+x],im->px[idx],aa);}
    continue;
   }
  }
  for(int x=first;x<last;x++){int ix=columns[x];if(ix<0)continue;int idx=iy*im->pw+ix;int aa=im->mask?opacity[im->mask[idx]]:a;if(!aa)continue;if(aa==32)framebuffer[y*256+x]=im->px[idx];else framebuffer[y*256+x]=blend(framebuffer[y*256+x],im->px[idx],aa);}
 }
 unsigned perf_us=(tickGetCount()-perf_start)*1000000/TICK_FREQ;perf_state[4]++;perf_state[5]+=perf_us;if(perf_us>perf_state[6])perf_state[6]=perf_us;
}
void cpymo_backend_image_draw(float dx,float dy,float dw,float dh,cpymo_backend_image image,int sx,int sy,int sw,int sh,float alpha,enum cpymo_backend_image_draw_type type){
 if(!nds_dual_accept(type))return;
 if(nds_dual_bottom_pass && nds_dual_is_say(type))dy+=nds_dual_say_offset();
 nds_clock_menu_rect(dx,dy,type);
 if(renderer_fast)image_draw_fast(dx,dy,dw,dh,image,sx,sy,sw,sh,alpha,type);
 else image_draw_reference(dx,dy,dw,dh,image,sx,sy,sw,sh,alpha,type);
}
static void fill_rects_impl(const float* r,size_t n,cpymo_color c,float alpha,enum cpymo_backend_image_draw_type type){
 if(n)nds_clock_menu_rect(r[0],r[1],type);
 (void)type;int a=(int)(alpha*32);u16 col=color(c);if(a<=0)return;
 float scale=display_scale(),ox=display_x(),oy=display_y();
 for(size_t i=0;i<n;i++,r+=4){int x0=(int)(r[0]*scale+ox),y0=(int)(r[1]*scale+oy),x1=(int)((r[0]+r[2])*scale+ox),y1=(int)((r[1]+r[3])*scale+oy);if(x0<0)x0=0;if(y0<0)y0=0;if(x1>256)x1=256;if(y1>192)y1=192;for(int y=y0;y<y1;y++){if(!(y&15))nds_audio_pump_if_due();for(int x=x0;x<x1;x++)framebuffer[y*256+x]=blend(framebuffer[y*256+x],col,a);}}
}
bool cpymo_backend_image_album_ui_writable(){return false;}
void cpymo_backend_image_fill_rects(const float* r,size_t n,cpymo_color c,float alpha,enum cpymo_backend_image_draw_type type){
 if(!nds_dual_accept(type))return;
 u64 t=tickGetCount();
 if(nds_dual_bottom_pass && nds_dual_is_say(type) && n){
  float off=nds_dual_say_offset();
  for(size_t i=0;i<n;i++){float shifted[4]={r[i*4],r[i*4+1]+off,r[i*4+2],r[i*4+3]};fill_rects_impl(shifted,1,c,alpha,type);}
 }else fill_rects_impl(r,n,c,alpha,type);
 fill_us+=(tickGetCount()-t)*1000000/TICK_FREQ;
}
#include "masktrans-nds.h"
static unsigned decode(const char** p){unsigned c=(unsigned char)*(*p)++;if(c<128)return c;unsigned n=c<224?1:c<240?2:3;c&=(1u<<(6-n))-1;while(n--){c=(c<<6)|((unsigned char)*(*p)++&63);}return c;}
typedef struct {u32 code;u8 advance,pad[3];u16 rows[14];} GameGlyph;
static GameGlyph* game_glyphs;static unsigned game_glyph_count;
#define BUILTIN_GLYPHS (sizeof glyph_codes/sizeof *glyph_codes)
static int glyph(unsigned c){if(game_glyphs){int lo=0,hi=game_glyph_count-1;while(lo<=hi){int m=(lo+hi)/2;if(game_glyphs[m].code==c)return BUILTIN_GLYPHS+m;if(game_glyphs[m].code<c)lo=m+1;else hi=m-1;}}int lo=0,hi=BUILTIN_GLYPHS-1;while(lo<=hi){int m=(lo+hi)/2;if(glyph_codes[m]==c)return m;if(glyph_codes[m]<c)lo=m+1;else hi=m-1;}return -1;}
static unsigned glyph_advance(int g){return g<(int)BUILTIN_GLYPHS?glyph_advances[g]:game_glyphs[g-BUILTIN_GLYPHS].advance;}
static unsigned glyph_row(int g,int y){return g<(int)BUILTIN_GLYPHS?glyph_rows[g][y]:game_glyphs[g-BUILTIN_GLYPHS].rows[y];}
static bool load_game_font(const char* dir){char path[512];snprintf(path,sizeof path,"%s/nds-cache/font.bin",dir);FILE* f=fopen(path,"rb");if(!f)return false;u32 header[2];if(fread(header,1,8,f)!=8||memcmp(header,"NF13",4)||!header[1]||header[1]>20000){fclose(f);return false;}unsigned count=header[1];GameGlyph* data=malloc(count*sizeof *data);if(!data){fclose(f);return false;}bool ok=fread(data,sizeof *data,count,f)==count&&fgetc(f)==EOF;fclose(f);for(unsigned i=0;ok&&i<count;i++){if((i&&data[i-1].code>=data[i].code)||data[i].advance>32)ok=false;for(unsigned y=0;y<14;y++)if(data[i].rows[y]&~8191u)ok=false;}if(!ok){free(data);return false;}free(game_glyphs);game_glyphs=data;game_glyph_count=count;engine_state[41]=count;return true;}
static FILE* trace_open(const char* mode);
static void missing_glyph(unsigned cp){engine_state[20]++;FILE* file=trace_open("ab");if(file){fprintf(file,"MISSING_GLYPH U+%04X\n",cp);fclose(file);}}
#include "ui-fallback-font.h"
#include "pixel-font-nds.h"
#include "clock-nds.h"
typedef struct {float size;unsigned mode;u8 name_bounds_ready,ink_top,ink_bottom,pad;char text[];} GlyphText;
float cpymo_backend_text_width(cpymo_str s,float size){unsigned mode=active_font_mode(),w=0,maxw=0;const char* p=s.begin;while(p<s.begin+s.len){unsigned c=decode(&p);if(c==13)continue;if(c==10){if(w>maxw)maxw=w;w=0;continue;}int g=text_glyph(mode,c);w+=g>=0?text_advance(mode,g):font_nominal(mode);}if(w>maxw)maxw=w;return maxw*(font_scale(size,mode)/display_scale());}
error_t cpymo_backend_text_create(cpymo_backend_text *out,float *width,cpymo_str s,float size){GlyphText* p=malloc(sizeof *p+s.len+1);if(!p)return CPYMO_ERR_OUT_OF_MEM;p->size=size;p->mode=active_font_mode();p->name_bounds_ready=0;memcpy(p->text,s.begin,s.len);p->text[s.len]=0;*out=p;*width=cpymo_backend_text_width(s,size);return CPYMO_ERR_SUCC;}
void cpymo_backend_text_free(cpymo_backend_text p){free(p);}
// Original PyMO centers name ink in its box. The core's fixed 0.9-em
// baseline assumes different font metrics; adapt only the live speaker name.
// Cache bounds on first draw: no repeated glyph scan or FAT access per frame.
static float name_baseline_adjustment(GlyphText* t,float scale,float screen_scale){
 if(!t->name_bounds_ready){
  unsigned top=font_height(t->mode),bottom=0;const char* p=t->text;
  while(*p){unsigned cp=decode(&p);if(cp==10||cp==13)continue;
   int g=text_glyph(t->mode,cp);if(g<0)g=text_glyph(t->mode,'?');
   if(g>=0)for(unsigned y=0;y<font_height(t->mode);y++)if(text_row(t->mode,g,y)){
    if(y<top)top=y;
    if(y+1>bottom)bottom=y+1;
   }
  }
  t->ink_top=top;t->ink_bottom=bottom;t->name_bounds_ready=1;
 }
 if(t->ink_bottom<=t->ink_top)return 0;
 // Box height = 1.4 em, supplied baseline = box top + 0.9 em.
 return (font_baseline(t->mode)-(t->ink_top+t->ink_bottom)*.5f)*scale
        -.2f*t->size*screen_scale;
}
static void text_draw_impl(cpymo_backend_text text,float x,float baseline,cpymo_color c,float alpha,enum cpymo_backend_image_draw_type type){
 GlyphText* t=text;unsigned mode=t->mode;const char* p=t->text;float screen_scale=display_scale();x=x*screen_scale+display_x();baseline=baseline*screen_scale+display_y();float scale=font_scale(t->size,mode);unsigned gh=font_height(mode),gw=font_width(mode),gb=font_baseline(mode);
 if(type==cpymo_backend_image_draw_type_text_say&&text==engine.say.name)
  baseline+=name_baseline_adjustment(t,scale,screen_scale);
 int h=(int)(gh*scale+.5f),w=(int)(gw*scale+.5f);
 if(scale==1.f){int pen=(int)x,top=(int)(baseline-gb);int a=(int)(alpha*32);u16 col=color(c);
  while(*p){unsigned cp=decode(&p);if(cp==13)continue;if(cp==10){pen=(int)x;top+=font_nominal(mode)+3;continue;}int g=text_glyph(mode,cp);if(g<0){missing_glyph(cp);g=text_glyph(mode,'?');}if(g>=0){for(unsigned y=0;y<gh;y++){int yy=top+y;if(yy<0||yy>=192)continue;unsigned bits=text_row(mode,g,y);for(unsigned xx=0;xx<gw;xx++)if(bits&(1<<xx)){int px=pen+xx;if(px>=0&&px<256){unsigned idx=yy*256+px;framebuffer[idx]=blend(framebuffer[idx],col,a);}}}pen+=text_advance(mode,g);}else pen+=font_nominal(mode);}
  return;
 }
 float start_x=x;while(*p){unsigned cp=decode(&p);if(cp==13)continue;if(cp==10){x=start_x;baseline+=(font_nominal(mode)+3)*scale;continue;}int g=text_glyph(mode,cp);if(g<0){missing_glyph(cp);g=text_glyph(mode,'?');}
 if(g>=0)for(int y=0;y<h;y++)for(int xx=0;xx<w;xx++){unsigned gy=(unsigned)(y/scale),gx=(unsigned)(xx/scale);if(gy<gh&&gx<gw&&(text_row(mode,g,gy)&(1<<gx)))pixel((int)x+xx,(int)(baseline-gb*scale)+y,c,alpha);}x+=(g>=0?text_advance(mode,g):font_nominal(mode))*scale;}
}
void cpymo_backend_text_draw(cpymo_backend_text text,float x,float baseline,cpymo_color c,float alpha,enum cpymo_backend_image_draw_type type){
 if(!nds_dual_accept(type))return;
 u64 t=tickGetCount();
 if(nds_dual_bottom_pass && nds_dual_is_say(type))baseline+=nds_dual_say_offset();
 text_draw_impl(text,x,baseline,c,alpha,type);
 text_us+=(tickGetCount()-t)*1000000/TICK_FREQ;
}
cpymo_input cpymo_input_snapshot(){
 int k=keysHeld();touchPosition t={0};if(k&KEY_TOUCH){touchRead(&t);presentation_state[8]++;}
 bool ui=cpymo_ui_enabled(&engine);
 // Script menus use the core cancel rule; save-enabled story choices keep X for the menu.
 bool script_menu=engine.select_img.selections&&!engine.select_img.save_enabled;
 cpymo_input in={0};in.ok=k&KEY_A;in.cancel=k&((ui||script_menu)?KEY_B:KEY_X);
 in.up=k&KEY_UP;in.down=k&KEY_DOWN;in.left=k&KEY_LEFT;in.right=k&KEY_RIGHT;in.skip=k&KEY_R;
 in.hide_window=!ui&&!engine.select_img.selections&&(k&KEY_B);
 /* Touch semantics follow the PHYSICAL lower LCD, not an assumed game
    coordinate space. Classic upper-game: whole auxiliary LCD = A.
    Classic lower-game: accurate PyMO coordinates including menus/choices.
    Enhanced: story area = A; core menus drawn below = real coordinates.
    A wake-up touch is blocked by nds_aux_poll when Y hid the sub LCD. */
 nds_touch_sample touch_sample=nds_touch_project(nds_display_mode,
   nds_dual_active(),ui,nds_dual_has_choice(),
   !!(k&KEY_TOUCH),nds_touch_blocked,!!(k&KEY_A),
   t.px,t.py,display_scale(),display_x(),display_y());
 in.ok=touch_sample.ok;
 in.mouse_button=touch_sample.mouse_button;
 in.mouse_position_useable=in.mouse_button;
 in.mouse_x=touch_sample.x;
 in.mouse_y=touch_sample.y;
 return in;
}
static FILE* savefile(const char* dir,const char* name,const char* mode){return nds_save_fopen(dir,name,mode);}
FILE* cpymo_backend_read_save(const char* dir,const char* name){return savefile(dir,name,"rb");}
FILE* cpymo_backend_write_save(const char* dir,const char* name){return savefile(dir,name,"wb");}
/* Movie wrapper is implemented after the power and framebuffer helpers. */
/* Release play does not synchronously open/write a card log for each line.
   Opt in with save/nds-debug.flag; telemetry remains available in RAM. */
volatile unsigned diagnostic_state[4]={0x4e444247};
static bool debug_trace;
/* Upstream stdout must not write tile-map bytes into our bitmap auxiliary
   screen. Diagnostic mode retains the original libnds console device. */
static const devoptab_t* console_output;
static devoptab_t filtered_output;
static ssize_t filtered_console_write(struct _reent* r,void* fd,const char* data,size_t size){
 if(debug_trace&&console_output&&console_output->write_r)return console_output->write_r(r,fd,data,size);
 return size;
}
static void filter_console_output(void){
 if(devoptab_list[STD_OUT]==&filtered_output)return;
 fflush(stdout);console_output=devoptab_list[STD_OUT];filtered_output=*console_output;
 filtered_output.write_r=filtered_console_write;devoptab_list[STD_OUT]=&filtered_output;
}
static FILE* trace_open(const char* mode){diagnostic_state[2]++;if(!debug_trace||!engine.assetloader.gamedir)return NULL;FILE* f=savefile(engine.assetloader.gamedir,"nds-trace.txt",mode);if(f)diagnostic_state[3]++;return f;}
void cpymo_backend_text_extract(const char* text){FILE* trace=trace_open("ab");if(trace){fprintf(trace,"TEXT %s\n",text);fclose(trace);}}
/* Keep a separate profiler so the existing 46-word acceptance ABI is stable. */
volatile unsigned menu_performance[5]={0x4d454e55};
extern error_t __real_cpymo_rmenu_enter(cpymo_engine*);
error_t __wrap_cpymo_rmenu_enter(cpymo_engine* e){
 nds_audio_update();u64 begin=tickGetCount();
 nds_registering_rmenu=true;error_t result=__real_cpymo_rmenu_enter(e);nds_registering_rmenu=false;
 unsigned us=(tickGetCount()-begin)*1000000/TICK_FREQ;
 menu_performance[1]++;menu_performance[2]=us;
 if(us>menu_performance[3])menu_performance[3]=us;
 nds_audio_update();FILE* trace=trace_open("ab");
 if(trace){fprintf(trace,"MENU_OPEN us=%u status=%d\n",us,result);fclose(trace);}
 return result;
}
/* magic, asleep, sleep/wake counts, last sleep ms, audio close/open,
   preserved tail samples, live source channels, underrun tails, wake dt us, polls */
volatile unsigned sleep_state[12]={0x4e534c31};
extern void nds_audio_suspend(void);
extern void nds_audio_resume(void);
static void nds_power_poll(void){sleep_state[11]++;unsigned wakes=sleep_state[3];if(!pmMainLoop())exit(0);if(wakes!=sleep_state[3])nds_aux_refresh();}
#include "theme-nds.h"
#include "launcher.h"
#include "aux-screen-nds.h"
#include "visual-capture-nds.h"
#include "dual-canvas-nds.h"
#include "nds-picture-bg.h"
/* Refined lower-LCD presentation for the 1.2 enhanced screen.
 * Native RGB555 decoration sits BEHIND the game's own speaker/name/text.
 * Do not stretch a captured upper screen or allocate a second framebuffer.
 * Menus/choices use a less decorated canvas to preserve hit-test contrast. */
static void nds_dual_canvas_audio_tick(void *unused){
 (void)unused;
 nds_audio_pump_if_due();
}
static void nds_dual_lower_background(bool ui,bool choices){
 const bool interactive=ui||choices;
 const bool dialogue_visible=engine.say.active &&
           !engine.input.hide_window && !engine.say.hide_window;
 /* Same RGB555 canvas function is compiled and pixel-tested on desktop;
    CPyMO's REAL namebox, textbox, text and menu render over it afterward. */
 /* Use the exact same ROM-embedded artwork under story, choices,
    restart and exit-to-library confirmations. Interactive pages get a
    25% dimming pass; otherwise a legacy plain-color panel would flash. */
 if(nds_picture_draw(framebuffer,interactive)){
  nds_audio_pump_if_due();
 }else{
  /* Unpatched developer ROM keeps the established canvas fallback. */
  nds_dual_canvas_draw_yield(framebuffer,interactive,dialogue_visible,
                             nds_dual_canvas_audio_tick,NULL);
 }
 if(!interactive && dialogue_visible)
  launcher_text(nds_ui("A / DOWN / 轻触：继续"),14,169,RGB15(19,25,28),248);
}
static PmEventCookie sleep_cookie;
static u64 sleep_started;
static void nds_power_event(void* user,PmEvent event){
 (void)user;
 if(event==PmEvent_OnSleep){sleep_started=tickGetCount();nds_audio_suspend();sleep_state[1]=1;sleep_state[2]++;}
 else if(event==PmEvent_OnWakeup){u64 delta=tickGetCount()-sleep_started;sleep_state[4]=delta*1000/TICK_FREQ;if(presentation_state[2])toast_started+=delta;nds_audio_resume();sleep_state[1]=0;sleep_state[3]++;}
}

#include "fvds-preflight-nds.h"
#include "movie-lz4-r7.h"
extern void __real_cpymo_charas_wait(cpymo_engine*);
extern void cpymo_charas_gc(cpymo_charas*,bool);
void __wrap_cpymo_charas_wait(cpymo_engine* e){
 if(nds_chara_effects_get()){__real_cpymo_charas_wait(e);return;}
 /* Finish one complete character command before drawing its final state.
    Image masks remain active; only timed character interpolation is skipped. */
 for(struct cpymo_chara* c=e->charas.chara;c;c=c->next){cpymo_tween_finish(&c->pos_x);cpymo_tween_finish(&c->pos_y);cpymo_tween_finish(&c->alpha);}
 cpymo_charas_gc(&e->charas,false);chara_effect_state[2]++;cpymo_engine_request_redraw(e);
}


volatile unsigned render_bench[68]={0x52424e43};
/* Private fixed mask benchmark: real DS code, both directions and endpoints. */
static void run_mask_bench(void){
 static const unsigned sizes[4][4]={{1,1,320,240},{256,192,480,320},{73,57,240,320},{137,91,4096,17}};
 int oldw=engine.gameconfig.imagesize_w,oldh=engine.gameconfig.imagesize_h;
 for(unsigned n=0;n<4;n++){
  MaskTransition* mask=mask_alloc(sizes[n][0],sizes[n][1]);
  if(!mask){render_bench[2]=CPYMO_ERR_OUT_OF_MEM;render_bench[1]=1;return;}
  for(unsigned i=0;i<mask->w*mask->h;i++)mask->pixels[i]=(i*37+i/13)&255;
  engine.gameconfig.imagesize_w=sizes[n][2];engine.gameconfig.imagesize_h=sizes[n][3];
  for(unsigned c=0;c<4;c++){
   unsigned total=0,maximum=0,hash=2166136261u;float t=c==0?0:c==1?.15f:c==2?.65f:1;
   for(unsigned repeat=0;repeat<16;repeat++){
    for(unsigned i=0;i<256*192;i++)framebuffer[i]=0x8000|((i*3+repeat*17)&0x7fff);
    u64 start=tickGetCount();cpymo_backend_masktrans_draw(mask,t,(n+c)&1);
    unsigned us=(tickGetCount()-start)*1000000/TICK_FREQ;total+=us;if(us>maximum)maximum=us;
    for(unsigned i=0;i<256*192;i++){hash^=framebuffer[i];hash*=16777619u;}
   }
   unsigned index=4+(n*4+c)*4;render_bench[index]=total;render_bench[index+1]=maximum;render_bench[index+2]=hash;render_bench[index+3]=16;render_bench[3]++;
  }
  cpymo_backend_masktrans_free(mask);
 }
 engine.gameconfig.imagesize_w=oldw;engine.gameconfig.imagesize_h=oldh;render_bench[1]=1;
}

static void run_render_bench(void){
 static const char* names[] = {"CC01_01M","CC01_02M","CC01_04M","CC01_05M"};
 for(unsigned n=0;n<4;n++){
  cpymo_backend_image image=NULL;int w,h;error_t err=cached_image(&image,&w,&h,cpymo_str_pure(names[n]),&engine.assetloader,"chara");
  if(err){render_bench[2]=err;render_bench[1]=1;return;}
  for(unsigned c=0;c<4;c++){
   unsigned total=0,maximum=0;float x=c==1?-80:c==2?80:0,y=c==2?-20:0,scale=c==3?.75f:1.f,a=c==1?.5f:1.f;
   for(unsigned repeat=0;repeat<16;repeat++){
    for(unsigned p=0;p<256*192;p++)framebuffer[p]=0x8010;
    u64 start=tickGetCount();
    cpymo_backend_image_draw(x,y,w*scale,h*scale,image,0,0,w,h,a,cpymo_backend_image_draw_type_chara);
    unsigned us=(tickGetCount()-start)*1000000/TICK_FREQ;total+=us;if(us>maximum)maximum=us;
   }
   unsigned hash=2166136261u;for(unsigned p=0;p<256*192;p++){hash^=framebuffer[p];hash*=16777619u;}
   unsigned index=4+(n*4+c)*4;render_bench[index]=total;render_bench[index+1]=maximum;render_bench[index+2]=hash;render_bench[index+3]=16;
   render_bench[3]++;
  }
  cpymo_backend_image_free(image);
 }
 render_bench[1]=1;
}


extern volatile unsigned audio_state[12],audio_bench[12];
extern volatile unsigned stream_diag[8];
extern void nds_stream_diag_reset(void);
volatile unsigned load_bench[40]={0x4c424e43};
static void run_load_bench(void){
 char names[4][64],kinds[4][16];
 FILE* spec=savefile(engine.assetloader.gamedir,"nds-bench-assets.txt","rb");
 if(!spec){load_bench[2]=1;load_bench[1]=1;return;}
 for(unsigned n=0;n<4;n++)if(fscanf(spec,"%15s %63s",kinds[n],names[n])!=2){fclose(spec);load_bench[2]=2;load_bench[1]=1;return;}
 fclose(spec);recent_limit=0;
 cpymo_audio_set_channel_volume(0,&engine.audio,.20f);cpymo_audio_set_channel_volume(1,&engine.audio,.20f);cpymo_audio_set_channel_volume(2,&engine.audio,.20f);
 load_bench[2]=cpymo_audio_bgm_play(&engine,cpymo_str_pure("BGM07"),true);
 load_bench[2]|=cpymo_audio_se_play(&engine,cpymo_str_pure("SE013"),true);
 load_bench[2]|=cpymo_audio_vo_play(&engine,cpymo_str_pure("8RD93O2Y"));
 nds_audio_update();nds_stream_diag_reset();for(unsigned i=1;i<8;i++)audio_state[i]=0;
 FILE* stress=savefile(engine.assetloader.gamedir,"nds-cache-stress.flag","rb");if(stress){fclose(stress);recent_limit=0;}recent_clear();for(unsigned i=1;i<32;i++)load_state[i]=0;for(unsigned i=1;i<8;i++)compressed_state[i]=0;
 engine_state[28]=0;perf_state[4]=perf_state[5]=perf_state[6]=0;
 for(unsigned repeat=0;repeat<8;repeat++)for(unsigned n=0;n<4;n++){
  u64 start=tickGetCount();cpymo_backend_image image=NULL;int w,h;
  error_t err=cached_image(&image,&w,&h,cpymo_str_pure(names[n]),&engine.assetloader,kinds[n]);
  if(err){load_bench[2]=err;load_bench[1]=1;return;}
  if(n==0)for(unsigned p=0;p<256*192;p++)framebuffer[p]=0x8010;
  cpymo_backend_image_draw(n==2?64:0,0,w,h,image,0,0,w,h,n==0?1.f:.5f,strcmp(kinds[n],"bg")?cpymo_backend_image_draw_type_chara:cpymo_backend_image_draw_type_bg);
  cpymo_backend_image_free(image);nds_audio_update();
  unsigned us=(tickGetCount()-start)*1000000/TICK_FREQ;
  load_bench[4+n]+=us;if(us>load_bench[8+n])load_bench[8+n]=us;
  unsigned hash=2166136261u;for(unsigned p=0;p<256*192;p++){hash^=framebuffer[p];hash*=16777619u;}
  load_bench[12+n]=hash;load_bench[3]++;
 }
 load_bench[16]=engine_state[28];load_bench[17]=perf_state[5];load_bench[18]=perf_state[6];
 for(unsigned i=1;i<8;i++)load_bench[20+i]=stream_diag[i];
 for(unsigned i=1;i<12;i++)audio_bench[i]=audio_state[i];load_bench[1]=1;
}


static void nds_base_draw(const cpymo_engine* e){cpymo_bg_draw(e);cpymo_scroll_draw(&e->scroll);cpymo_charas_draw(e);cpymo_anime_draw(&e->anime);}
static void nds_engine_draw(const cpymo_engine* e){
 nds_menu_clock_visible=false;clock_state[9]=0;
 if(cpymo_ui_enabled(e)){
  /* Keep the live authored scene as the upper canvas. In contrast to the
     first prototype, also render UI_BG on top (as CPyMO 3DS does), while
     interactive UI_ELEMENT/UI_ELEMENT_BG are routed to the lower LCD.
     This avoids an empty black top for menus lacking their own UI_BG. */
  if(nds_dual_active()&&!nds_dual_bottom_pass)nds_base_draw(e);
  cpymo_engine_draw(e);
  return;
 }
 scene_count=0;scene_overflow=false;scene_collect=true;nds_base_draw(e);scene_collect=false;
 float view[3]={display_scale(),display_x(),display_y()};
 nds_audio_pump_if_due();
 bool same=!scene_overflow&&scene_previous_count==scene_count&&!memcmp(scene_view,view,sizeof view)&&!memcmp(scene_previous,scene_commands,scene_count*sizeof(SceneCommand));
 bool hit=scene_valid&&same;
 if(hit){memcpy(framebuffer,scene_pixels,sizeof framebuffer);scene_state[1]++;}
 else{
  nds_base_draw(e);scene_state[2]++;
  /* Avoid copying a changing scene: populate only after two identical sequences. */
  if(same&&!scene_pixels){extern void* __real_malloc(size_t);scene_pixels=__real_malloc(sizeof framebuffer);}
  if(same&&scene_pixels){memcpy(scene_pixels,framebuffer,sizeof framebuffer);scene_valid=true;}else{scene_valid=false;if(same)scene_state[3]++;}
  memcpy(scene_previous,scene_commands,scene_count*sizeof(SceneCommand));memcpy(scene_view,view,sizeof view);scene_previous_count=scene_count;
 }
 nds_audio_pump_if_due();
 cpymo_select_img_draw(&e->select_img,e->gameconfig.imagesize_w,e->gameconfig.imagesize_h,e->gameconfig.grayselected);
 cpymo_floating_hint_draw(&e->floating_hint);cpymo_flash_draw(e);cpymo_bg_draw_transform_effect(e);cpymo_fade_draw(e);cpymo_text_draw(e);cpymo_say_draw(e);
}

/* Read-only runtime telemetry for hardware acceptance tests.
 * [1] actual TWL runtime (not simply DSi console), [2] linked-memory profile.
 * This deliberately does not modify CPU clock, SCFG or heap bounds. */
volatile unsigned nds_runtime_state[5]={0x43505457,0,0,0,0};
#ifdef CPYMO_DSI_MEMORY_LAYOUT
/* Nonzero TWL section makes the experimental ROM DSi-capable.
 * Touch it only after hardware confirms TWL mode. */
static volatile u32 nds_twl_probe_slot[256] TWL_BSS __attribute__((used));
#endif
int main(int argc,char** argv){
 nds_runtime_state[1]=isDSiMode()?1u:0u;
 nds_runtime_state[3]=(unsigned)getHeapStart();
 nds_runtime_state[4]=(unsigned)getHeapLimit();
#ifdef CPYMO_DSI_MEMORY_LAYOUT
 if(nds_runtime_state[1]){
  nds_twl_probe_slot[0]=0x44534931u;
  nds_runtime_state[2]=nds_twl_probe_slot[0]==0x44534931u?1u:2u;
 }
#endif
 pmAddEventHandler(&sleep_cookie,nds_power_event,NULL);
 /* Start Calico's lazy tick service through its public thread API before
    the launcher uses tickGetCount(); audio used to initialize it later. */
 threadSleepTicks(1);
 /* Initialize the bitmap screens before accessing FAT, but do not draw the
    localized game-library loading screen with firmware language. The saved
    CPyMO preference is only available after the card is mounted. */
 debug_trace=false;nds_language_boot();filter_console_output();launcher_video_init();
 if(!fatInitDefault()){launcher_failure_hint("无法访问存储卡",CPYMO_ERR_CAN_NOT_OPEN_FILE,"请关机后重新启动");launcher_video_show();while(1){nds_power_poll();swiWaitForVBlank();}}
 nds_language_load();nds_global_preferences_load();nds_theme_load();
 /* First visible game-list transition now honors the resolved CPyMO locale,
    including the Chinese default on English/Japanese DSi firmware. */
 launcher_show_splash();
 const char* hint=argc>1?argv[1]:NULL;
 for(;;){
 debug_trace=false;filter_console_output();
 char game_dir[256];launcher_choose(game_dir,sizeof game_dir,hint);
 render_bits=3;{FILE* policy=savefile(game_dir,"nds-render-policy.txt","rb");if(policy){int value=fgetc(policy);fclose(policy);if(value>='0'&&value<='3')render_bits=value-'0';}}render_policy[1]=render_bits;
 FILE* debug_flag=savefile(game_dir,"nds-debug.flag","rb");debug_trace=debug_flag!=NULL;if(debug_flag)fclose(debug_flag);diagnostic_state[1]=debug_trace;diagnostic_state[2]=diagnostic_state[3]=0;
 recent_clear();recent_limit=nds_recent_image_budget();
 nds_bg_cache_enabled=recent_limit>=RECENT_LIMIT_TWL;
 /* Optional test override: game root/nds-bg-cache.txt containing 0 disables
  * background caching only; character caching and game assets are unchanged. */
 FILE* bg_cache_flag=savefile(game_dir,"nds-bg-cache.txt","rb");
 if(bg_cache_flag){if(fgetc(bg_cache_flag)=='0')nds_bg_cache_enabled=false;fclose(bg_cache_flag);}
 for(unsigned i=1;i<5;i++)background_cache_state[i]=0;
 memset(&engine,0,sizeof engine);memset((void*)(engine_state+1),0,sizeof engine_state-sizeof engine_state[0]);
 if(debug_trace)consoleDemoInit();
 nds_cache_enabled=true;
 nds_library_exit_requested=false;nds_title_restart_pending=false;
 nds_dual_load(game_dir);
 nds_capture_init(game_dir);
 error_t err=(load_game_font(game_dir)&&load_pixel_fonts(game_dir))?CPYMO_ERR_SUCC:CPYMO_ERR_BAD_FILE_FORMAT;
 FILE* sequential_file=savefile(game_dir,"nds-image-sequential.txt","rb");image_sequential=!sequential_file||fgetc(sequential_file)!='0';if(sequential_file)fclose(sequential_file);
 char layout_path[512],layout_marker[32]={0};snprintf(layout_path,sizeof layout_path,"%s/nds-cache/format.txt",game_dir);FILE* marker_file=fopen(layout_path,"rb");if(marker_file){fread(layout_marker,1,31,marker_file);fclose(marker_file);}int format_layout=!strcmp(layout_marker,"CPYMO-NDS-CACHE-6\n")?3:0;
 FILE* layout_file=savefile(game_dir,"nds-layout-policy.txt","rb");int layout=layout_file?fgetc(layout_file)-'0':format_layout;if(layout_file)fclose(layout_file);cache_layout_policy=layout>=0&&layout<=3?layout:0;
 FILE* fast_file=savefile(game_dir,"nds-render-fast.txt","rb");renderer_fast=!fast_file||fgetc(fast_file)!='0';if(fast_file)fclose(fast_file);
 FILE* row_file=savefile(game_dir,"nds-row-copy.txt","rb");native_row_copy=row_file&&fgetc(row_file)!='0';if(row_file)fclose(row_file);
 FILE* buffer_file=savefile(game_dir,"nds-image-buffer.txt","rb");int buffer_policy=buffer_file?fgetc(buffer_file)-'0':2;if(buffer_file)fclose(buffer_file);image_buffer_policy=buffer_policy>=0&&buffer_policy<=3?buffer_policy:0;
 extern void nds_stream_guard_policy(bool);FILE* guard_file=savefile(game_dir,"nds-refill-guard.txt","rb");bool guard=!guard_file||fgetc(guard_file)!='0';if(guard_file)fclose(guard_file);nds_stream_guard_policy(guard);
 if(!err)err=cpymo_engine_init(&engine,game_dir);engine_state[1]=err;
 // DS readability policy is in memory only. Keep layout line spacing in
 // agreement with the selected native pixel size; never rewrite gameconfig.txt.
 if(!err){nds_preferences_load(game_dir);enforce_text_readability();if(nds_cache_enabled)warm_menu_images();}
 if(debug_trace)printf("Core init: %d\n",err);FILE* trace=trace_open("wb");if(trace){fprintf(trace,"INIT %d\n",err);fclose(trace);}
 if(err){launcher_state[6]=err;cpymo_audio_free(&engine.audio);if(debug_trace)printf("Game init failed: %d\nPress B to return to games.\n",err);else launcher_failure("启动失败，请检查转换结果",err);do{nds_power_poll();swiWaitForVBlank();scanKeys();}while(!(keysDown()&KEY_B));continue;}
 free(scene_verify_pixels);scene_verify_pixels=NULL;FILE* verify_flag=savefile(game_dir,"nds-scene-verify.flag","rb");if(verify_flag){fclose(verify_flag);scene_verify_pixels=malloc(sizeof framebuffer);scene_state[7]=scene_verify_pixels!=NULL;}
 FILE* mask_flag=savefile(game_dir,"nds-maskbench.flag","rb");if(mask_flag){fclose(mask_flag);run_mask_bench();for(;;)swiWaitForVBlank();}
 FILE* bench_flag=savefile(game_dir,"nds-renderbench.flag","rb");if(bench_flag){fclose(bench_flag);run_render_bench();for(;;)swiWaitForVBlank();}
 nds_playing=true;nds_display_start();
 nds_touch_role_state=(nds_touch_role_tracker){0};
 nds_aux_refresh();
 if(debug_trace){consoleClear();printf("CPyMO Nintendo DS\n\nA / Touch: Next\nX: Menu    B: Hold to hide text\nR: Skip read text\nSTART: Save slot 1\nSELECT: Load slot 1\n\n%s\n",nds_runtime_mode_label());}else launcher_play_panel(NULL);
 // Count actual display interrupts; include missed frames during decoding/drawing.
 irqSet(IRQ_VBLANK,on_vblank);irqEnable(IRQ_VBLANK);
 FILE* load_flag=savefile(game_dir,"nds-loadbench.flag","rb");if(load_flag){fclose(load_flag);run_load_bench();for(;;){swiWaitForVBlank();nds_audio_update();}}
 u64 prev_tick=tickGetCount(),observed_tick=prev_tick;float elapsed=0;char last_script[64]={0};
 while(1){unsigned wakes=sleep_state[3];nds_power_poll();bool woke=sleep_state[3]!=wakes;if(woke)prev_tick=observed_tick=tickGetCount();swiWaitForVBlank();scanKeys();
 nds_audio_update();
 if(!debug_trace)launcher_update_toast();
 /* Three screen modes by a single L press:
    classic scene top -> classic scene bottom -> enhanced split -> repeat. */
 if(keysDown()&KEY_L)nds_display_cycle();
 /* Persist only the final L-cycled mode, not every transient selection. */
 nds_dual_poll_mode();
 nds_aux_poll();
 /* UI/choice changes may alter pen semantics while it is still down.
    Resolve the boundary before the core copies its previous input. */
 nds_touch_prepare_input((unsigned)keysHeld());
 nds_clock_poll();
 if(keysDown()&KEY_START){error_t save_err=cpymo_save_write(&engine,1);engine_state[10]=(unsigned)save_err;engine_state[12]++;if(!debug_trace)launcher_toast(save_err?"快速保存到存档1失败":"已快速保存到存档1");trace=trace_open("ab");if(trace){fprintf(trace,"SAVE slot=1 status=%d\n",save_err);fclose(trace);}}
 if((keysDown()&KEY_SELECT)&&!(keysHeld()&KEY_L)){FILE* saved=cpymo_save_open_read(&engine,1);error_t load_err=CPYMO_ERR_CAN_NOT_OPEN_FILE;if(saved){load_err=cpymo_save_load_savedata(&engine,saved);fclose(saved);}engine_state[11]=(unsigned)load_err;engine_state[13]++;if(!debug_trace)launcher_toast(load_err?"读取存档1失败或存档不存在":"已快速读取存档1");trace=trace_open("ab");if(trace){fprintf(trace,"LOAD slot=1 status=%d\n",load_err);fclose(trace);}if(saved&&load_err){engine_state[1]=(unsigned)load_err;break;}}
 unsigned now=vblank_count;u64 now_tick=tickGetCount();float dt=(float)(now_tick-prev_tick)/TICK_FREQ;prev_tick=now_tick;if(woke)sleep_state[10]=(unsigned)(dt*1000000);elapsed+=dt;bool redraw=false;
 bool saved_skip_policy=engine.config_skip_already_read_only;
 bool skip_override=false; /* L is now exclusively screen swap; skip policy comes from settings. */
 if(skip_override)engine.config_skip_already_read_only=false;
 unsigned interval_us=(now_tick-observed_tick)*1000000/TICK_FREQ;observed_tick=now_tick;
 engine_state[33+(interval_us<=20000?0:interval_us<=34000?1:interval_us<=50000?2:interval_us<=100000?3:4)]++;
 u64 update_start=tickGetCount();loaded_image=false;
 err=cpymo_engine_update(&engine,dt,&redraw);
 if(nds_library_exit_requested){
  /* The confirmation callback has already exited its UI. Leave the
     engine alive until the common teardown below; do not restart ROM. */
  trace=trace_open("ab");
  if(trace){fputs("EXIT_TO_LIBRARY\n",trace);fclose(trace);}
  engine_state[1]=0;
  break;
 }
 if(!err&&nds_title_restart_pending){
  nds_title_restart_pending=false;
  nds_dual_title_reset(&nds_title_gate);
  nds_menu_clock_visible=false;clock_state[9]=0;
  nds_aux_clock_suppressed=false;
  nds_aux_refresh();
  cpymo_engine_request_redraw(&engine);
  redraw=true;
 }
 if(!err){
  enforce_text_readability();
  /* Only narrative dialogue ends the opening/title screen. No script-name
     heuristic, input remap, or changes to PyMO's authored title assets. */
  unsigned title_ended=nds_dual_title_observe(&nds_title_gate,
      engine.say.active,cpymo_ui_enabled(&engine),
      engine.select_img.selections!=NULL);
  if(title_ended){
    if(nds_display_mode==2){
   /* Previous sub LCD held auxiliary help. The upcoming split pass will
      replace it with the real CPyMO dialogue on the same frame. */
   nds_aux_refresh();
   cpymo_engine_request_redraw(&engine);
   redraw=true;
    }else if(!debug_trace)launcher_play_panel(NULL);
   }
 }
 if(skip_override)engine.config_skip_already_read_only=saved_skip_policy;engine_state[1]=err;engine_state[2]++;
 nds_preferences_sync();
 /* Rebuild UI objects after their updater returns, never from a setter while
    its data is still in use. Recreate the parent menu too, so Back is localized. */
 if(nds_language_take_refresh()){
  while(cpymo_ui_enabled(&engine))cpymo_ui_exit(&engine);
  err=cpymo_rmenu_enter(&engine);if(!err)err=cpymo_config_ui_enter(&engine);
  if(!err)cpymo_list_ui_set_selection_relative_to_cur(&engine,8);
  engine_state[1]=err;launcher_play_panel(NULL);redraw=true;
 }
 control_state[1]=(engine.say.auto_mode?1:0)|(engine.say.hide_window?2:0)|(engine.input.hide_window?4:0)|((engine.skipping||engine.input.skip)?8:0)|(engine.config_skip_already_read_only?16:0);control_state[2]=engine.interpreter?engine.interpreter->checkpoint.cur_line:0;
 if(loaded_image){prev_tick=tickGetCount();engine_state[32]++;}
 unsigned update_us=(tickGetCount()-update_start)*1000000/TICK_FREQ;if(update_us>engine_state[31])engine_state[31]=update_us;
 engine_state[5]=(unsigned)(elapsed*1000);engine_state[6]=(unsigned)now;
 if(err==CPYMO_ERR_NO_MORE_CONTENT){trace=trace_open("ab");if(trace){fprintf(trace,"SESSION_END\n");fclose(trace);}engine_state[1]=0;break;}
 if(err){if(!debug_trace){launcher_failure("运行失败，请检查游戏资源",err);do{nds_power_poll();swiWaitForVBlank();scanKeys();}while(!(keysDown()&KEY_B));}else printf("Update error: %d\n",err);trace=trace_open("ab");if(trace){fprintf(trace,"ERROR %d\n",err);fclose(trace);}break;}
 if(engine.interpreter && engine.interpreter->script){
  engine_state[14]=(unsigned)engine.interpreter->checkpoint.cur_line;
  const char* name=engine.interpreter->script->script_name;
  if(strcmp(last_script,name)){snprintf(last_script,sizeof last_script,"%s",name);trace=trace_open("ab");if(trace){fprintf(trace,"SCRIPT %s\n",name);fclose(trace);}}
  /* Generic startup-menu marker; script names are recorded in the trace. */
  engine_state[7]=!strcmp(name,"MENU")?10:0;
 }
 if(redraw){unsigned image_before=perf_state[5],text_before=text_us,fill_before=fill_us;u64 draw_start=tickGetCount();for(unsigned i=0;i<256*192;i++)framebuffer[i]=BIT(15);nds_dual_bottom_pass=false;nds_engine_draw(&engine);if(scene_verify_pixels){memcpy(scene_verify_pixels,framebuffer,sizeof framebuffer);for(unsigned i=0;i<256*192;i++)framebuffer[i]=BIT(15);unsigned saved_bits=render_bits;bool saved_copy=native_row_copy,saved_fast=renderer_fast;render_bits=0;native_row_copy=false;renderer_fast=false;cpymo_engine_draw(&engine);render_bits=saved_bits;native_row_copy=saved_copy;renderer_fast=saved_fast;scene_state[5]++;if(memcmp(scene_verify_pixels,framebuffer,sizeof framebuffer))scene_state[6]++;}
  /* Brief save/load or settings feedback while enhanced owns the lower
     LCD. The regular auxiliary panel still owns classic-mode toasts.
     Draw only on the upper GAME canvas, after the source scene and its
     optional verification; never bake this overlay into cached scene data. */
  if(nds_dual_active() && presentation_state[2] && nds_toast_text[0]){
   launcher_rect(0,0,256,24,RGB15(3,5,8));
   launcher_text(nds_toast_text,10,4,RGB15(31,31,31),246);
  }
  // DMA reads main memory, not the ARM9 data cache. Publish the whole frame.
  nds_clock_sync_aux_visibility();
  DC_FlushRange(framebuffer,sizeof framebuffer);
  dmaCopy(framebuffer,vram,sizeof framebuffer);engine_state[29]++;
  /* In classic-lower mode the sub BG lives on the PHYSICAL upper LCD.
     Snapshot the original help/clock VRAM first, then this exact main
     framebuffer on the lower LCD. No extra 96 KiB staging allocation. */
  if(nds_capture_armed && nds_display_mode==1 && screens_swapped &&
     launcher_sub && !cpymo_ui_enabled(&engine) && !nds_dual_has_choice() &&
     engine.say.active && !engine.say.hide_window && !engine.input.hide_window){
   FILE* classic=nds_capture_begin(4,launcher_sub);
   nds_capture_finish(classic,4,framebuffer);
  }
  if(nds_dual_active() && launcher_sub){
   /* Save the exact rendered pixels only when explicitly armed by a file
      in this game's save directory. Never render synthetic preview data. */
   /* Capture story choices separately: a selection may coexist with the
      previous dialogue state, but its true touch targets belong below. */
   unsigned capture_kind=cpymo_ui_enabled(&engine)?2:
     nds_dual_has_choice()?3:
     (engine.say.active&&!engine.say.hide_window&&
      !engine.input.hide_window)?1:0;
   FILE* capture=nds_capture_begin(capture_kind,framebuffer);
   /* Render only say / UI on the lower LCD, preserving the current scene
      on the upper LCD and the existing VRAM and ARM7 owners. No extra heap
      framebuffer; reuse the RAM draw surface after publishing the top. */
   nds_dual_bottom_pass=true;
   nds_dual_lower_background(cpymo_ui_enabled(&engine),nds_dual_has_choice());
   if(cpymo_ui_enabled(&engine))cpymo_engine_draw(&engine);
   else{
    cpymo_say_draw(&engine);
    if(nds_dual_has_choice())cpymo_select_img_draw(&engine.select_img,
       engine.gameconfig.imagesize_w,engine.gameconfig.imagesize_h,
       engine.gameconfig.grayselected);
   }
   DC_FlushRange(framebuffer,sizeof framebuffer);
   dmaCopy(framebuffer,launcher_sub,sizeof framebuffer);
   nds_capture_finish(capture,capture_kind,framebuffer);
   nds_dual_bottom_pass=false;
   nds_dual_state[6]++;
  }
  /* Commit a pending physical LCD role swap only AFTER both target VRAM
     surfaces have been published. This avoids exposing the previous
     auxiliary panel as a one-frame lower-screen ghost in mode 1 -> 2. */
  nds_display_commit_pending_swap();
  if(engine.bg.transform_draw){engine_state[38]++;float p=cpymo_tween_value(&engine.bg.transform_progression);if(p>0&&p<1)engine_state[39]|=1u<<(unsigned)(p*31);}
  unsigned draw_us=(tickGetCount()-draw_start)*1000000/TICK_FREQ;if(draw_us>engine_state[30])engine_state[30]=draw_us;if(draw_us>50000){unsigned seq=draw_trace[1]++,j=4+(seq%64)*8;draw_trace[j]=seq;draw_trace[j+1]=draw_us;draw_trace[j+2]=perf_state[5]-image_before;draw_trace[j+3]=text_us-text_before;draw_trace[j+4]=fill_us-fill_before;draw_trace[j+5]=engine_state[14];draw_trace[j+6]=engine_state[2];draw_trace[j+7]=mask_state[3];}}
 nds_audio_update();
 struct mallinfo mi=mallinfo();engine_state[3]=mi.uordblks;if(engine_state[3]>engine_state[4])engine_state[4]=engine_state[3];
 if(false && engine_state[2]%60==0)printf("\x1b[6;0HFrame: %6u    \nHeap: %7u      \nPeak: %7u      ",engine_state[2],engine_state[3],engine_state[4]);
 if(engine_state[2]%120==0){trace=trace_open("ab");if(trace){fprintf(trace,"STATE frame=%u ms=%u vblank=%u bg=%s\n",engine_state[2],engine_state[5],engine_state[6],engine.bg.current_bg_name?engine.bg.current_bg_name:"none");fclose(trace);}}
 }
 nds_playing=false;nds_display_restore_launcher();nds_aux_refresh();cpymo_engine_free(&engine);
 }
}
