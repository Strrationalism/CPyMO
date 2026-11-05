#include "fvds-stream-nds.h"
#include <string.h>
#include <limits.h>

static uint16_t u16le(const uint8_t *a){return (uint16_t)a[0]|(uint16_t)((uint16_t)a[1]<<8);}
static uint32_t u32le(const uint8_t *a){return (uint32_t)u16le(a)|((uint32_t)u16le(a+2)<<16);}
static int read_exact(FILE *f,uint8_t *buf,size_t count){return fread(buf,1,count,f)==count;}
static int goto_offset(FILE *f,uint32_t offset){return offset<=INT32_MAX && !fseek(f,(long)offset,SEEK_SET);}
static uint32_t cursor(FILE *f){long n=ftell(f);return n<0?UINT32_MAX:(uint32_t)n;}

int nds_fvds_open(nds_fvds_stream *s,const char *filename){
 uint8_t header[28],key[8];uint32_t prior_frame=0,prior_pos=0;
 if(!s||!filename)return -1;
 memset(s,0,sizeof(*s));
 s->file=fopen(filename,"rb");
 if(!s->file)return -2;
 if(fseek(s->file,0,SEEK_END))goto invalid;
 long size=ftell(s->file);
 if(size<36||size>192L*1024L*1024L||!goto_offset(s->file,0))goto invalid;
 s->bytes=(uint32_t)size;
 if(!read_exact(s->file,header,sizeof header)||memcmp(header,"FVDS",4))goto invalid;
 s->width=u16le(header+4);s->height=u16le(header+6);
 s->fps_num=u32le(header+8);s->fps_den=u32le(header+12);
 s->sample_rate=u16le(header+16);s->channels=u16le(header+18);
 s->frame_count=u32le(header+20);s->keyframe_count=u32le(header+24);
 if(s->width!=256||s->height!=192||!s->fps_num||s->fps_num>1000||
    !s->fps_den||s->fps_den>1000||s->sample_rate!=47605||s->channels!=2||
    !s->frame_count||s->frame_count>18000||!s->keyframe_count||
    s->keyframe_count>512||s->keyframe_count>s->frame_count)goto invalid;
 s->data_offset=28+s->keyframe_count*8;
 if((uint64_t)s->data_offset+s->frame_count*4>(uint64_t)s->bytes)goto invalid;
 for(uint32_t i=0;i<s->keyframe_count;i++){
  if(!read_exact(s->file,key,8))goto invalid;
  uint32_t frame=u32le(key),pos=u32le(key+4);
  if((i==0&&(frame||pos!=s->data_offset))||
     (i>0&&(frame<=prior_frame||pos<=prior_pos))||
     frame>=s->frame_count||pos>=s->bytes)goto invalid;
  prior_frame=frame;prior_pos=pos;
 }
 if(!goto_offset(s->file,s->data_offset))goto invalid;
 s->next_key_index=0;
 s->next_key_frame=0;
 s->next_key_offset=s->data_offset;
 return 0;
invalid:
 nds_fvds_close(s);
 return -3;
}
void nds_fvds_close(nds_fvds_stream *s){
 if(!s)return;
 if(s->file)fclose(s->file);
 memset(s,0,sizeof(*s));
}
int nds_fvds_next(nds_fvds_stream *s,nds_fvds_packet *pkt){
 uint8_t word[4];
 if(!s||!s->file||!pkt||s->packet_open)return -1;
 if(s->frame_index==s->frame_count)
  return cursor(s->file)==s->bytes?1:-5;
 uint32_t pos=cursor(s->file);
 s->current_is_keyframe=(s->frame_index==s->next_key_frame);
 if(s->current_is_keyframe){
  /* The header index must point to actual packet boundaries, not merely
     monotonically increasing offsets. Otherwise a bad seek could interpret
     compressed payload bytes as packet lengths and overrun buffers. */
  if(pos!=s->next_key_offset)return -6;
  if(s->next_key_index+1<s->keyframe_count){
   uint8_t key[8];
   if(!goto_offset(s->file,28+(s->next_key_index+1)*8) ||
      !read_exact(s->file,key,8) ||
      !goto_offset(s->file,pos))return -7;
   s->next_key_index++;
   s->next_key_frame=u32le(key);
   s->next_key_offset=u32le(key+4);
  }else{
   s->next_key_index=s->keyframe_count;
   s->next_key_frame=s->frame_count;
   s->next_key_offset=s->bytes;
  }
 }
 if(pos==UINT32_MAX||pos>s->bytes||s->bytes-pos<4||
    !read_exact(s->file,word,4))return -2;
 uint32_t value=u32le(word);
 uint32_t video=value&0x1ffffu,blocks=value>>17;
 if(!video||(video&3)||!blocks||blocks>255)return -3;
 uint32_t audio=blocks*(uint32_t)s->channels*132;
 uint32_t data_pos=cursor(s->file);
 if(data_pos==UINT32_MAX||(uint64_t)data_pos+video+audio>s->bytes)return -4;
 pkt->frame=s->frame_index;
 pkt->video_bytes=video;pkt->audio_bytes=audio;pkt->audio_blocks=blocks;
 s->video_remaining=video;s->audio_remaining=audio;
 s->packet_end=data_pos+video+audio;
 s->packet_open=1;
 s->frame_index++;
 return 0;
}
size_t nds_fvds_read_video(nds_fvds_stream *s,void *buf,size_t capacity){
 if(!s||!s->file||!s->packet_open||!buf)return 0;
 if(capacity>s->video_remaining)capacity=s->video_remaining;
 size_t got=fread(buf,1,capacity,s->file);
 s->video_remaining-=(uint32_t)got;
 return got;
}
size_t nds_fvds_read_audio(nds_fvds_stream *s,void *buf,size_t capacity){
 if(!s||!s->file||!s->packet_open||!buf||s->video_remaining)return 0;
 if(capacity>s->audio_remaining)capacity=s->audio_remaining;
 size_t got=fread(buf,1,capacity,s->file);
 s->audio_remaining-=(uint32_t)got;
 return got;
}
int nds_fvds_finish(nds_fvds_stream *s){
 if(!s||!s->file||!s->packet_open)return -1;
 /* Decoder may skip remaining payload on B, EOF or a corrupt frame.
    A failed seek is always fatal and must not publish an old frame. */
 if(!goto_offset(s->file,s->packet_end))return -2;
 s->video_remaining=0;s->audio_remaining=0;s->packet_open=0;
 return 0;
}
int nds_fvds_seek_keyframe(nds_fvds_stream *s,uint32_t index){
 uint8_t key[8];
 if(!s||!s->file||index>=s->keyframe_count)return -1;
 if(!goto_offset(s->file,28+index*8)||!read_exact(s->file,key,8))return -2;
 uint32_t frame=u32le(key),pos=u32le(key+4);
 if(frame>=s->frame_count||pos<s->data_offset||pos>=s->bytes||
    !goto_offset(s->file,pos))return -3;
 s->frame_index=frame;s->packet_open=0;s->video_remaining=0;s->audio_remaining=0;
 s->next_key_index=index;s->next_key_frame=frame;s->next_key_offset=pos;
 return 0;
}
