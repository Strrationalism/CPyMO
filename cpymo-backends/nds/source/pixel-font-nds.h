/* NDS native bitmap fonts. Backend code AGPL-3.0-or-later; glyph data OFL-1.1. */
typedef struct {u32 code;u8 advance,pad[3];u16 rows[16];} PixelGlyph;
typedef struct {PixelGlyph* data;unsigned count,width,height,baseline,nominal;} PixelFont;
static PixelFont pixel_fonts[2];
static unsigned font_choice;
/* magic, available, choice, changes, setting write error, small count, large count */
volatile unsigned font_state[7]={0x4e504631};
static void free_pixel_fonts(void){for(unsigned i=0;i<2;i++){free(pixel_fonts[i].data);memset(&pixel_fonts[i],0,sizeof pixel_fonts[i]);}font_state[1]=0;}
static bool read_pixel_font(PixelFont* out,const char* dir,const char* name,unsigned nominal){
 char path[512];snprintf(path,sizeof path,"%s/nds-cache/font-%s.bin",dir,name);FILE* f=fopen(path,"rb");if(!f)return false;
 struct {char magic[4];u32 count;u8 width,height,baseline,nominal;} h;
 bool ok=fread(&h,1,sizeof h,f)==sizeof h&&!memcmp(h.magic,"NPF1",4)&&h.count>0&&h.count<=20000&&h.width==16&&h.height==16&&h.baseline<=16&&h.nominal==nominal;
 PixelGlyph* data=ok?malloc(h.count*sizeof *data):NULL;ok=ok&&data;
 if(ok)ok=fread(data,sizeof *data,h.count,f)==h.count&&fgetc(f)==EOF;
 fclose(f);
 for(unsigned i=0;ok&&i<h.count;i++)if((i&&data[i-1].code>=data[i].code)||data[i].advance>32)ok=false;
 if(!ok){free(data);return false;}
 *out=(PixelFont){data,h.count,h.width,h.height,h.baseline,h.nominal};return true;
}
static bool load_pixel_fonts(const char* dir){
 free_pixel_fonts();font_choice=0;memset((void*)(font_state+1),0,sizeof font_state-sizeof font_state[0]);
 char path[512];snprintf(path,sizeof path,"%s/nds-cache/font-small.bin",dir);FILE* f=fopen(path,"rb");
 if(!f)return true; /* Old converted packages retain NF13 support. */
 fclose(f);
 if(!read_pixel_font(&pixel_fonts[0],dir,"small",12)||!read_pixel_font(&pixel_fonts[1],dir,"large",16)){free_pixel_fonts();return false;}
 font_state[1]=1;font_state[5]=pixel_fonts[0].count;font_state[6]=pixel_fonts[1].count;
 snprintf(path,sizeof path,"%s/save/nds-font.cfg",dir);f=fopen(path,"rb");if(f){char setting[16]={0};fread(setting,1,sizeof setting-1,f);fclose(f);if(!strcmp(setting,"NDSFONT1 1\n"))font_choice=1;}
 font_state[2]=font_choice;return true;
}
int nds_font_choices_available(void){return font_state[1]!=0;}
int nds_font_choice_get(void){return font_choice;}
static unsigned text_native_pixels(void){return nds_font_choices_available()?pixel_fonts[font_choice].nominal:13;}
void nds_font_choice_set(int choice){
 if(!nds_font_choices_available()||choice<0||choice>1)return;
 font_choice=choice;font_state[2]=choice;font_state[3]++;enforce_text_readability();
 char path[512];snprintf(path,sizeof path,"%s/save/nds-font.cfg",engine.assetloader.gamedir);FILE* f=fopen(path,"wb");bool ok=false;
 if(f){ok=fprintf(f,"NDSFONT1 %u\n",font_choice)>0;if(fclose(f))ok=false;}font_state[4]=!ok;
}
static unsigned active_font_mode(void){return nds_font_choices_available()?font_choice+1:0;}
static unsigned text_glyph_base(unsigned mode){return mode?pixel_fonts[mode-1].count:BUILTIN_GLYPHS+game_glyph_count;}
static int text_glyph(unsigned mode,unsigned cp){
 int g=-1;if(!mode)g=glyph(cp);else{PixelFont* f=&pixel_fonts[mode-1];int lo=0,hi=f->count-1;while(lo<=hi){int m=(lo+hi)/2;if(f->data[m].code==cp){g=m;break;}if(f->data[m].code<cp)lo=m+1;else hi=m-1;}}
 if(g>=0)return g;int fallback=ui_fallback_glyph(cp);return fallback<0?-1:text_glyph_base(mode)+fallback;
}
static unsigned text_advance(unsigned mode,int g){unsigned base=text_glyph_base(mode);if((unsigned)g>=base)return mode==2?ui_large_advances[g-base]:ui_fallback_advances[g-base];return mode?pixel_fonts[mode-1].data[g].advance:glyph_advance(g);}
static unsigned text_row(unsigned mode,int g,int y){unsigned base=text_glyph_base(mode);if((unsigned)g>=base)return mode==2?ui_large_rows[g-base][y]:ui_fallback_rows[g-base][y];return mode?pixel_fonts[mode-1].data[g].rows[y]:glyph_row(g,y);}
static unsigned font_nominal(unsigned mode){return mode?pixel_fonts[mode-1].nominal:13;}
static unsigned font_width(unsigned mode){return mode?16:13;}
static unsigned font_height(unsigned mode){return mode?16:14;}
static unsigned font_baseline(unsigned mode){return mode?pixel_fonts[mode-1].baseline:11;}
static float font_scale(float size,unsigned mode){
 if(mode){/* Preserve the bitmap grid, including authored large text. */
  int integer=(int)(size*display_scale()/font_nominal(mode)+.5f);return integer<1?1:integer;
 }
 float pixels=(int)(size*display_scale()+.5f);return (pixels<13?13:pixels)/13.f;
}
