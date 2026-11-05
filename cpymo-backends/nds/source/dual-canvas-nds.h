/* CPyMO 1.2 enhanced lower-LCD canvas, source-identical host/ARM9 drawing.
 * This is BACKDROP ONLY: game-authored textbox, fonts, choices and menus
 * are drawn by CPyMO on top of it. No generated illustrations or stand-ins.
 * Standard C; no libnds headers, heap allocation or disk I/O.
 */
#ifndef CPYMO_NDS_DUAL_CANVAS_H
#define CPYMO_NDS_DUAL_CANVAS_H
#include <stdint.h>
#include <stddef.h>
#define NDS_DUAL_CANVAS_W 256
#define NDS_DUAL_CANVAS_H 192

static inline uint16_t nds_dual_canvas_color(unsigned r,unsigned g,unsigned b){
 return (uint16_t)(0x8000u | (r&31u) | ((g&31u)<<5) | ((b&31u)<<10));
}
/* A+B refinement: quiet, RGB555-native navy gradient instead of four
 * abrupt 48-line stripes. Dialogue only occupies the TOP portion of the
 * touch LCD in actual PyMO games, so the bottom stays intentionally clean:
 * no fabricated footer or horizontal bar beneath the original textbox.
 * Integer-only math; a scanline uses ONE color, avoiding spatial dithering
 * noise over game-owned text. Keep this backdrop platform-independent. */
static inline uint16_t nds_dual_canvas_row(unsigned y){
 if(y>=NDS_DUAL_CANVAS_H)y=NDS_DUAL_CANVAS_H-1;
 unsigned bottom=(NDS_DUAL_CANVAS_H-1)-y;
 const unsigned half=(NDS_DUAL_CANVAS_H-1)/2;
 return nds_dual_canvas_color(
    (5u*bottom+2u*y+half)/(NDS_DUAL_CANVAS_H-1),
    (9u*bottom+4u*y+half)/(NDS_DUAL_CANVAS_H-1),
    (13u*bottom+7u*y+half)/(NDS_DUAL_CANVAS_H-1));
}
/* The balanced 1+3 art direction: the original quiet navy reading panel
 * remains the base. A small, deterministic field of dusk stars and two soft
 * cloud horizons give the unoccupied lower LCD a little visual-novel mood.
 * Entirely 256x192 RGB555; no file I/O, heap, second buffer or animation.
 * Story assets always render AFTER this backdrop. */
static inline unsigned nds_dual_canvas_horizon(const uint8_t* knots,unsigned x){
 const unsigned slot=x>>4, frac=x&15u;
 return ((unsigned)knots[slot]*(16u-frac)+(unsigned)knots[slot+1]*frac+8u)>>4;
}
static inline uint16_t nds_dual_canvas_cloud(unsigned y,int foreground){
 const uint16_t col=nds_dual_canvas_row(y);
 const unsigned r=col&31u,g=(col>>5)&31u,b=(col>>10)&31u;
 return foreground
  ? nds_dual_canvas_color(r?r-1u:0u,g,b+1u)
  : nds_dual_canvas_color(r+1u,g+1u,b+2u);
}
typedef struct {uint8_t x,y,light;} nds_dual_canvas_star;
static inline void nds_dual_canvas_draw_yield(uint16_t *pixels,
                                        int interactive,int dialogue_visible,
                                        void (*audio_tick)(void*),void* audio_ctx){
 (void)dialogue_visible;
 if(!pixels)return;
 for(unsigned y=0;y<NDS_DUAL_CANVAS_H;y++){
  const uint16_t col=nds_dual_canvas_row(y);
  for(unsigned x=0;x<NDS_DUAL_CANVAS_W;x++)
   pixels[y*NDS_DUAL_CANVAS_W+x]=col;
  /* Preserve six per-frame audio servicing opportunities on ARM9. */
  if(audio_tick && (y&31u)==0)audio_tick(audio_ctx);
 }
 /* Both dialogue and interactive modes keep the familiar thin top edge.
  * It sits outside the game's own name / message box. */
 const uint16_t bright=nds_dual_canvas_color(12,18,23);
 const uint16_t muted=nds_dual_canvas_color(7,12,17);
 for(unsigned x=12;x<244;x++)pixels[7*NDS_DUAL_CANVAS_W+x]=muted;
 for(unsigned x=12;x<38;x++)pixels[7*NDS_DUAL_CANVAS_W+x]=bright;
 if(interactive)return; /* settings, save/load and choices stay distraction-free */

 /* Low-contrast distant clouds, below the usual PyMO textbox region.
  * Interpolated height knots avoid jagged random/noisy pixel textures. */
 const uint8_t far_h[17]={
   148,147,144,143,145,149,151,148,145,143,145,149,147,143,142,145,148
 };
 const uint8_t near_h[17]={
   171,169,166,165,168,173,174,171,167,166,168,173,170,166,164,167,170
 };
 for(unsigned x=0;x<NDS_DUAL_CANVAS_W;x++){
  const unsigned distant=nds_dual_canvas_horizon(far_h,x);
  const unsigned near=nds_dual_canvas_horizon(near_h,x);
  for(unsigned y=distant;y<near;y++)
   pixels[y*NDS_DUAL_CANVAS_W+x]=nds_dual_canvas_cloud(y,0);
  for(unsigned y=near;y<NDS_DUAL_CANVAS_H;y++)
   pixels[y*NDS_DUAL_CANVAS_W+x]=nds_dual_canvas_cloud(y,1);
 }

 /* Calm sparse pinpoints instead of a noisy star field. Most are below the
  * active reading area or near the physical edges, never flashing/animated. */
 static const nds_dual_canvas_star stars[]={
   {25,38,0},{230,34,0},{238,65,1},{20,84,0},{229,91,0},
   {236,114,0},{17,123,0},{52,128,0},{93,134,0},{132,125,1},
   {176,132,0},{205,126,1},{224,138,0},{36,140,1},
   {71,143,0},{151,139,0},{187,144,0},{215,146,0}
 };
 const uint16_t dim=nds_dual_canvas_color(6,10,14);
 const uint16_t star=nds_dual_canvas_color(9,14,19);
 for(unsigned i=0;i<sizeof stars/sizeof stars[0];i++){
  const nds_dual_canvas_star* p=&stars[i];
  pixels[p->y*NDS_DUAL_CANVAS_W+p->x]=p->light?star:dim;
  if(p->light){
   pixels[(p->y-1u)*NDS_DUAL_CANVAS_W+p->x]=dim;
   pixels[p->y*NDS_DUAL_CANVAS_W+p->x+1u]=dim;
  }
 }
 /* A single faint shooting star at the far edge alludes to concept #3.
  * Unlike the concept illustration it is only 11 muted RGB555 pixels. */
 for(unsigned i=0;i<11;i++)
  pixels[(108u+i/3u)*NDS_DUAL_CANVAS_W+(206u-i)]=dim;
 pixels[108u*NDS_DUAL_CANVAS_W+206u]=star;

 /* Quiet inner-frame treatment from concept #1. It is deliberately weaker
  * than the textbox border and must never act as a second dialogue box. */
 for(unsigned y=16;y<181;y++){
  const uint16_t base=nds_dual_canvas_row(y);
  const unsigned r=base&31u,g=(base>>5)&31u,b=(base>>10)&31u;
  const uint16_t edge=nds_dual_canvas_color(r+1u,g+2u,b+2u);
  pixels[y*NDS_DUAL_CANVAS_W+7]=edge;
  pixels[y*NDS_DUAL_CANVAS_W+248]=edge;
 }
 const uint16_t corner=nds_dual_canvas_color(6,9,13);
 for(unsigned x=12;x<30;x++){
  pixels[181*NDS_DUAL_CANVAS_W+x]=corner;
  pixels[181*NDS_DUAL_CANVAS_W+(255u-x)]=corner;
 }
}
static inline void nds_dual_canvas_draw(uint16_t *pixels,
                                        int interactive,int dialogue_visible){
 nds_dual_canvas_draw_yield(pixels,interactive,dialogue_visible,NULL,NULL);
}
#endif
