/* CPyMO for NDS 1.2 - original FVDS coded-picture header auditor.
 * Host-side diagnostic, never used as a picture decoder or video player.
 * It consumes each picture's first 7 bits through the SAME bounded bit
 * reader intended for ARM9, and validates the I/P result against the
 * container's independently checked keyframe index. No game art output.
 */
#include "fvds-picture-source-nds.h"
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

int main(int argc,char **argv){
 if(argc!=2){
  fprintf(stderr,"usage: fvds-picture-audit video.fv\n");
  return 2;
 }
 nds_fvds_stream file;
 int result=nds_fvds_open(&file,argv[1]);
 if(result){
  fprintf(stderr,"FVDS open failed: %d\n",result);
  return 3;
 }
 uint32_t indexed_i=0, predicted=0, first_q=0, min_q=64,max_q=0;
 uint64_t video_bytes=0, audio_bytes=0;
 int success=1;
 for(uint32_t frame=0;frame<file.frame_count;frame++){
  nds_fvds_packet pkt;nds_fvds_bits bits;
  result=nds_fvds_next(&file,&pkt);
  if(result||!nds_fvds_picture_begin(&file,&bits)){
   fprintf(stderr,"Invalid picture packet at frame %" PRIu32 "\n",frame);
   success=0;break;
  }
  uint32_t type=0,quantizer=0;
  if(!nds_fvds_bits_read(&bits,1,&type)){
   fprintf(stderr,"Truncated picture flag at frame %" PRIu32 "\n",frame);
   success=0;break;
  }
  if(type==0){
   if(!file.current_is_keyframe||
      !nds_fvds_bits_read(&bits,6,&quantizer)){
    fprintf(stderr,"Unindexed or incomplete I frame %" PRIu32 "\n",frame);
    success=0;break;
   }
   if(!indexed_i)first_q=quantizer;
   if(quantizer<min_q)min_q=quantizer;
   if(quantizer>max_q)max_q=quantizer;
   indexed_i++;
   printf("I %" PRIu32 " q=%" PRIu32 " bytes=%" PRIu32 "\n",
          frame,quantizer,pkt.video_bytes);
  }else{
   if(file.current_is_keyframe){
    fprintf(stderr,"Predicted picture in indexed key slot %" PRIu32 "\n",frame);
    success=0;break;
   }
   predicted++;
  }
  video_bytes+=pkt.video_bytes;
  audio_bytes+=pkt.audio_bytes;
  /* Skip the rest of this packet: do not interpret compressed DCT as audio. */
  if(nds_fvds_finish(&file)){
   fprintf(stderr,"Cannot finish picture %" PRIu32 "\n",frame);
   success=0;break;
  }
 }
 if(success && nds_fvds_next(&file,&(nds_fvds_packet){0})!=1)
  success=0;
 if(success && (indexed_i!=file.keyframe_count ||
                 (uint64_t)indexed_i+predicted!=file.frame_count))success=0;
 if(success)
  printf("PASS frames=%" PRIu32 " I=%" PRIu32 " P=%" PRIu32
         " q_min=%" PRIu32 " q_max=%" PRIu32
         " q_first=%" PRIu32 " video_bytes=%" PRIu64
         " audio_bytes=%" PRIu64 "\n",
         file.frame_count,indexed_i,predicted,
         min_q,max_q,first_q,video_bytes,audio_bytes);
 nds_fvds_close(&file);
 return success?0:4;
}
