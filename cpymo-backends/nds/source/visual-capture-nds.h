/* CPyMO NDS 1.2: opt-in native LCD framebuffer capture for genuine
 * hardware visual review. Does not draw UI, hook input or allocate RAM.
 * Creates up to four physical-screen pairs: enhanced dialogue/menu/choice,
 * and a classic-lower dialogue. Each capture is written only once per game.
 * Capture is fully DISABLED unless the user creates:
 *    <game>/save/nds-visual-capture.flag
 *
 * NDSPIX1 layout (little-endian, 16 byte header):
 *  [0..7]  ASCII "NDSPIX1\0"
 *  [8..9]  width=256  [10..11] height=192
 *  [12..13] frame_count=2 [14..15] kind=1(enhanced dialogue),
 *  2(enhanced menu), 3(enhanced story choices), 4(classic lower dialogue).
 *  [16..]  TOP RGB555+bit15, then BOTTOM RGB555+bit15, row major.
 * Pixels come from the exact CPyMO frame as presented to libnds DMA.
 * The host converter creates PNGs without scaling or reinterpreting art.
 */
#ifndef CPYMO_NDS_VISUAL_CAPTURE_H
#define CPYMO_NDS_VISUAL_CAPTURE_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static char nds_capture_directory[300];
static unsigned nds_capture_armed, nds_capture_taken, nds_capture_error;
static char nds_capture_path[512];

static void nds_capture_init(const char* game_dir){
 nds_capture_armed=nds_capture_taken=nds_capture_error=0;
 nds_capture_directory[0]=0;
 if(!game_dir)return;
 int n=snprintf(nds_capture_path,sizeof nds_capture_path,
                "%s/save/nds-visual-capture.flag",game_dir);
 if(n<0||(size_t)n>=sizeof nds_capture_path)return;
 FILE* f=fopen(nds_capture_path,"rb");
 if(!f)return;
 fclose(f);
 n=snprintf(nds_capture_directory,sizeof nds_capture_directory,"%s",game_dir);
 if(n<0||(size_t)n>=sizeof nds_capture_directory){
  nds_capture_directory[0]=0;return;
 }
 nds_capture_armed=1;
}
static int nds_capture_pixels(FILE* f,const u16* pixels){
 /* Eight scanlines per write; keep the engine audio pump responsive while
    performing exceptional SD-card I/O. No additional 96 KiB buffer. */
 for(unsigned y=0;y<192;y+=8){
  if(fwrite(pixels+y*256,sizeof(u16),8*256,f)!=8*256)return 0;
  nds_audio_pump_if_due();
 }
 return 1;
}
static FILE* nds_capture_begin(unsigned kind,const u16* top){
 if(!nds_capture_armed||kind<1||kind>4||
    (nds_capture_taken&(1u<<(kind-1))))return NULL;
 const char* names[]={"dialogue","menu","choice","classic-lower"};
 int n=snprintf(nds_capture_path,sizeof nds_capture_path,
   "%s/save/nds-capture-%s.ndsraw",nds_capture_directory,names[kind-1]);
 if(n<0||(size_t)n>=sizeof nds_capture_path)return NULL;
 FILE* f=fopen(nds_capture_path,"wb");
 if(!f){nds_capture_error++;return NULL;}
 const unsigned char magic[8]={'N','D','S','P','I','X','1',0};
 const uint16_t meta[4]={256,192,2,(uint16_t)kind};
 if(fwrite(magic,1,sizeof magic,f)!=sizeof magic ||
    fwrite(meta,sizeof meta[0],4,f)!=4 ||
    !nds_capture_pixels(f,top)){
  fclose(f);remove(nds_capture_path);nds_capture_error++;return NULL;
 }
 return f;
}
static void nds_capture_finish(FILE* f,unsigned kind,const u16* bottom){
 if(!f)return;
 int ok=nds_capture_pixels(f,bottom);
 if(fclose(f))ok=0;
 if(ok)nds_capture_taken|=(1u<<(kind-1));
 else{remove(nds_capture_path);nds_capture_error++;}
}
#endif
