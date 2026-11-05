/* Standalone host-side WAV reference exporter for CPyMO FVDS audio.
 * Builds from our independently implemented packet reader + block decoder.
 * Usage: fvds-to-wav input.fv output.wav
 * This program DOES NOT decode video and is NOT embedded into the .nds.
 */
#include "fvds-stream-nds.h"
#include "fvds-adpcm-nds.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int word16(FILE*f,uint16_t value){
 return fputc(value&255,f)!=EOF && fputc(value>>8,f)!=EOF;
}
static int word32(FILE*f,uint32_t value){
 return word16(f,(uint16_t)value) && word16(f,(uint16_t)(value>>16));
}
static int wav_header(FILE*f,uint32_t data_size,uint32_t rate){
 if(fseek(f,0,SEEK_SET))return 0;
 return fwrite("RIFF",1,4,f)==4 && word32(f,data_size+36) &&
  fwrite("WAVEfmt ",1,8,f)==8 &&
  word32(f,16)&&word16(f,1)&&word16(f,2)&&word32(f,rate)&&
  word32(f,rate*4)&&word16(f,4)&&word16(f,16)&&
  fwrite("data",1,4,f)==4&&word32(f,data_size);
}
int main(int argc,char**argv){
 if(argc!=3){fprintf(stderr,"usage: fvds-to-wav input.fv output.wav\n");return 2;}
 /* Never truncate the source: output may alias input via symlink or
    hardlink even when the supplied path strings differ. */
 struct stat source_stat, output_stat;
 if(!strcmp(argv[1],argv[2]) ||
    (!stat(argv[1],&source_stat) && !stat(argv[2],&output_stat) &&
     source_stat.st_dev==output_stat.st_dev &&
     source_stat.st_ino==output_stat.st_ino)){
  fprintf(stderr,"Source and destination refer to the same file\n");
  return 2;
 }
 nds_fvds_stream input;
 if(nds_fvds_open(&input,argv[1])){
  fprintf(stderr,"Invalid or inaccessible FVDS source\n");return 3;
 }
 FILE*out=fopen(argv[2],"wb");
 if(!out){nds_fvds_close(&input);return 4;}
 int ok=wav_header(out,0,input.sample_rate);
 uint64_t output_bytes=0;
 uint8_t block[264],video[4096],pcm[1024];
 nds_fvds_packet p;
 while(ok){
  int rc=nds_fvds_next(&input,&p);
  if(rc==1)break;
  if(rc!=0){ok=0;break;}
  unsigned remaining=p.video_bytes;
  while(ok&&remaining){
   size_t amount=remaining>sizeof(video)?sizeof(video):remaining;
   size_t got=nds_fvds_read_video(&input,video,amount);
   if(got!=amount)ok=0;
   else remaining-=(unsigned)got;
  }
  for(uint32_t i=0;ok&&i<p.audio_blocks;i++){
   if(nds_fvds_read_audio(&input,block,sizeof block)!=sizeof block){ok=0;break;}
   int16_t stereo[512];
   if(nds_fvds_adpcm_pair(block,sizeof block,stereo,512)!=256){ok=0;break;}
   /* Write little endian PCM regardless of host byte order. */
   for(unsigned j=0;j<512;j++){
    uint16_t v=(uint16_t)stereo[j];
    pcm[2*j]=(uint8_t)v;pcm[2*j+1]=(uint8_t)(v>>8);
   }
   if(output_bytes+sizeof pcm>UINT32_MAX-36 ||
      fwrite(pcm,1,sizeof pcm,out)!=sizeof pcm){ok=0;break;}
   output_bytes+=sizeof pcm;
  }
  if(ok && nds_fvds_finish(&input))ok=0;
 }
 if(ok)ok=wav_header(out,(uint32_t)output_bytes,input.sample_rate);
 if(fclose(out))ok=0;
 nds_fvds_close(&input);
 if(!ok){remove(argv[2]);fprintf(stderr,"FVDS payload or output I/O error\n");return 5;}
 fprintf(stderr,"Decoded FVDS stereo PCM: %llu bytes\n",
   (unsigned long long)output_bytes);
 return 0;
}
