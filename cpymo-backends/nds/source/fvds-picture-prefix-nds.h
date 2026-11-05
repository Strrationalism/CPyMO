/* FastVideoDS FVDS picture-prefix metadata, independently parsed.
 * This does NOT decode 4x4 DCT, motion vectors or RGB555 pixels.
 * The first little-endian 16-bit word carries the I/P high-bit flag.
 */
#ifndef CPYMO_NDS_FVDS_PICTURE_PREFIX_H
#define CPYMO_NDS_FVDS_PICTURE_PREFIX_H
#include <stddef.h>
#include <stdint.h>
/* -1: too short; -2: indexed keyframe starts with P picture
 *  0: I-picture; 1: P-picture. */
static inline int nds_fvds_picture_type(const uint8_t *bytes,size_t length,
                                       int indexed_keyframe){
 if(!bytes||length<2)return -1;
 unsigned prefix=(unsigned)bytes[0]|((unsigned)bytes[1]<<8);
 int predicted=(prefix&0x8000u)?1:0;
 if(indexed_keyframe&&predicted)return -2;
 return predicted;
}
/* For an I-picture, the first bit is 0 and the next six bits contain
 * a quantizer scale: MSB-first within a LITTLE-ENDIAN 16-bit word.
 * P-pictures have another header layout and no I quantizer at this spot.
 * Returns -1 for incomplete/P pictures, 0..63 for an I quantizer. */
static inline int nds_fvds_intra_quantizer(const uint8_t *bytes,size_t length){
 if(!bytes||length<2)return -1;
 unsigned header=(unsigned)bytes[0]|((unsigned)bytes[1]<<8);
 if(header&0x8000u)return -1;
 return (int)((header>>9)&0x3fu);
}
#endif
