/* Native DS mask transitions; matching the upstream SDL mask/radius curve.
   Backend code AGPL-3.0-or-later. */
typedef struct {unsigned w,h;unsigned char pixels[];} MaskTransition;
/* magic, cache loads, raw loads, draws, direction bits, progression bits, load errors */
volatile unsigned mask_state[9]={0x4e4d5431};
static MaskTransition* mask_alloc(unsigned w,unsigned h){
 if(!w||!h||w>256||h>192)return NULL;
 MaskTransition* m=malloc(sizeof *m+w*h);if(m){m->w=w;m->h=h;}return m;
}
error_t cpymo_backend_masktrans_create(cpymo_backend_masktrans *out,void* pixels,int w,int h){
 if(!pixels||w<=0||h<=0||w>4096||h>4096)return CPYMO_ERR_BAD_FILE_FORMAT;
 unsigned dw=(unsigned)(engine.gameconfig.imagesize_w*display_scale()+.5f),dh=(unsigned)(engine.gameconfig.imagesize_h*display_scale()+.5f);
 MaskTransition* m=mask_alloc(dw,dh);if(!m)return CPYMO_ERR_OUT_OF_MEM;
 for(unsigned y=0;y<dh;y++){if(!(y&15))nds_audio_pump_if_due();for(unsigned x=0;x<dw;x++)m->pixels[y*dw+x]=((unsigned char*)pixels)[(y*(unsigned)h/dh)*w+x*(unsigned)w/dw];}
 free(pixels);*out=m;mask_state[2]++;return CPYMO_ERR_SUCC;
}
void cpymo_backend_masktrans_free(cpymo_backend_masktrans m){free(m);}
extern error_t __real_cpymo_assetloader_load_system_masktrans(cpymo_backend_masktrans*,cpymo_str,const cpymo_assetloader*);
error_t __wrap_cpymo_assetloader_load_system_masktrans(cpymo_backend_masktrans* out,cpymo_str name,const cpymo_assetloader* loader){
 char path[512];nds_cache_path(path,sizeof path,loader->gamedir,"mask",name,".nmt");FILE* f=nds_cache_enabled?fopen(path,"rb"):NULL;
 if(!f)return __real_cpymo_assetloader_load_system_masktrans(out,name,loader);
 struct {char magic[4];u16 w,h;} header;
 bool ok=fread(&header,1,8,f)==8&&!memcmp(header.magic,"NMT1",4)&&header.w&&header.h&&header.w<=256&&header.h<=192;
 MaskTransition* m=ok?mask_alloc(header.w,header.h):NULL;ok=ok&&m;
 if(ok){unsigned count=header.w*header.h;ok=fread(m->pixels,1,count,f)==count&&fgetc(f)==EOF;}
 fclose(f);nds_audio_update();if(!ok){free(m);mask_state[6]++;return CPYMO_ERR_BAD_FILE_FORMAT;}
 *out=m;mask_state[1]++;return CPYMO_ERR_SUCC;
}
void cpymo_backend_masktrans_draw(cpymo_backend_masktrans mask,float t,bool fade_in){
 MaskTransition* m=mask;if(!m)return;if(t<0)t=0;if(t>1)t=1;
 unsigned char opacity[256];int threshold=(int)(t*765+.5f);
 for(int value=0;value<256;value++){int a=(threshold-2*value)*32/255;if(a<0)a=0;if(a>32)a=32;if(t<=0)a=0;if(t>=1)a=32;opacity[value]=fade_in?32-a:a;}
 int x0=(int)(display_x()+.5f),y0=(int)(display_y()+.5f),dw=(int)(engine.gameconfig.imagesize_w*display_scale()+.5f),dh=(int)(engine.gameconfig.imagesize_h*display_scale()+.5f);
 unsigned columns[256];bool mapped=(render_bits&2)&&dw>0&&dw<=256;if(mapped){for(int x=0;x<dw;x++)columns[x]=x*m->w/dw;render_policy[5]++;}
 for(int y=0;y<dh;y++){if(!(y&15))nds_audio_pump_if_due();int py=y0+y;if(py<0||py>=192)continue;unsigned row=y*m->h/dh*m->w;
  for(int x=0;x<dw;x++){int px=x0+x;if(px<0||px>=256)continue;int a=opacity[m->pixels[row+(mapped?columns[x]:(unsigned)x*m->w/dw)]];unsigned i=py*256+px;framebuffer[i]=blend(framebuffer[i],BIT(15),a);}}
 mask_state[3]++;mask_state[4]|=fade_in?2:1;mask_state[5]|=1u<<(unsigned)(t*31);mask_state[7]=(unsigned)(t*1000);mask_state[8]=fade_in;
}
