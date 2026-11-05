/* Independent IMA-style quantized delta reconstruction for FVDS.
 * Each channel block has a little-endian signed predictor and a 16-bit
 * index followed by 128 low-nibble-first packed bytes (256 PCM samples).
 * Index transition -3,-3,-2,-1,+2,+4,+6,+8 is the FVDS bitstream rule.
 */
#include "fvds-adpcm-nds.h"

static const uint16_t step_size[89]={
 7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,
 50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,
 230,253,279,307,337,371,408,449,494,544,598,658,724,796,
 876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,
 2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,
 8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,
 22385,24623,27086,29794,32767
};
static const int8_t index_jump[8]={-3,-3,-2,-1,2,4,6,8};

static int16_t clamp_pcm(int value){
 if(value>32767)value=32767;
 if(value< -32767)value=-32767;
 return (int16_t)value;
}
/* Decode the channel into its interleaved output lane.
 * The caller validates both indexes before writing either lane. */
static void channel(const uint8_t *data,int16_t *dest,unsigned lane){
 int pred=(int16_t)((unsigned)data[0]|((unsigned)data[1]<<8));
 int index=(int)((unsigned)data[2]|((unsigned)data[3]<<8));
 unsigned written=0;
 for(unsigned i=4;i<NDS_FVDS_ADPCM_BLOCK_BYTES;i++){
  unsigned byte=data[i];
  for(unsigned part=0;part<2;part++){
   unsigned nibble=part?(byte>>4):(byte&15u);
   unsigned magnitude=nibble&7u;
   int delta=((int)step_size[index]*(int)(2u*magnitude+1u))>>3;
   pred=clamp_pcm((nibble&8u)?pred-delta:pred+delta);
   dest[2*written+lane]=(int16_t)pred;
   written++;
   index+=index_jump[magnitude];
   if(index<0)index=0;else if(index>88)index=88;
  }
 }
}
int nds_fvds_adpcm_pair(const uint8_t *in,size_t bytes,
                       int16_t *out_interleaved,size_t out_samples){
 if(!in||!out_interleaved||bytes!=NDS_FVDS_ADPCM_PAIR_BYTES||
    out_samples<NDS_FVDS_ADPCM_INTERLEAVED_SAMPLES)return -1;
 const uint8_t *right=in+NDS_FVDS_ADPCM_BLOCK_BYTES;
 unsigned left_index=(unsigned)in[2]|((unsigned)in[3]<<8);
 unsigned right_index=(unsigned)right[2]|((unsigned)right[3]<<8);
 if(left_index>=89||right_index>=89)return -2;
 channel(in,out_interleaved,0);
 channel(right,out_interleaved,1);
 return NDS_FVDS_ADPCM_SAMPLES_PER_BLOCK;
}
