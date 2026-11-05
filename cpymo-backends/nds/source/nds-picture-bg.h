/* CPyMO for NDS 1.2: truly single-ROM enhanced lower-LCD artwork.
 *
 * One 256x192 RGB555 bitmap is embedded directly in the ARM9 ROM image.
 * The build reserves a deterministic, 98320-byte slot; tools/patch-rom-bg.py
 * replaces that slot with the owner's approved RGB555 artwork without
 * shifting addresses, changing executable code, or requiring an extra file.
 *
 * No FAT access, decoder, per-frame loading or dynamic heap allocation.
 * In unpatched developer builds, a distinct staging marker makes
 * nds_picture_get() return NULL and retains the safe procedural canvas.
 */
#ifndef CPYMO_NDS_PICTURE_BG_H
#define CPYMO_NDS_PICTURE_BG_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NDS_PICTURE_PIXELS (256u * 192u)
#define NDS_PICTURE_MARKER_SIZE 16u
#define NDS_PICTURE_WAIT_MARKER "CPYMO_BG_AWAIT12"
#define NDS_PICTURE_READY_MARKER "CPYMO_BG_READY12"
#define NDS_PICTURE_PLACEHOLDER 0x5a5au

typedef struct {
 unsigned char marker[NDS_PICTURE_MARKER_SIZE];
 uint16_t pixels[NDS_PICTURE_PIXELS];
} nds_picture_embedded_record;

/* GCC's designated range initializer emits the entire reserved image in
 * immutable ROM .rodata, not zero-initialized BSS. Volatile marker reads
 * below prevent constant folding of the staging marker by -O2/LTO. */
static const nds_picture_embedded_record nds_picture_embedded
 __attribute__((used,aligned(4))) = {
 .marker={'C','P','Y','M','O','_','B','G','_','A','W','A','I','T','1','2'},
 .pixels={[0 ... NDS_PICTURE_PIXELS-1] = NDS_PICTURE_PLACEHOLDER}
};

#define nds_picture_pixels (nds_picture_embedded.pixels)

static inline const uint16_t* nds_picture_get(void){
 const volatile unsigned char* found=
    (const volatile unsigned char*)nds_picture_embedded.marker;
 static const unsigned char wanted[NDS_PICTURE_MARKER_SIZE]={
    'C','P','Y','M','O','_','B','G','_','R','E','A','D','Y','1','2'};
 for(unsigned i=0;i<NDS_PICTURE_MARKER_SIZE;i++)
  if(found[i]!=wanted[i])return NULL;
 return nds_picture_embedded.pixels;
}
/* UI decisions use the same artwork as ordinary dialogue, with a mild
 * 1/4 RGB555 dimming for contrasting white menu text and touch targets.
 * The existing CPyMO confirmation/menu elements are drawn on top. */
static inline uint16_t nds_picture_dim_color(uint16_t native){
 unsigned r=(native&31u)*3u/4u;
 unsigned g=((native>>5)&31u)*3u/4u;
 unsigned b=((native>>10)&31u)*3u/4u;
 return (uint16_t)(0x8000u|r|(g<<5)|(b<<10));
}
static inline bool nds_picture_draw(uint16_t* target,bool interactive){
 const uint16_t* source=nds_picture_get();
 if(!source||!target)return false;
 for(unsigned i=0;i<NDS_PICTURE_PIXELS;i++)
  target[i]=interactive?nds_picture_dim_color(source[i]):source[i];
 return true;
}
#endif
