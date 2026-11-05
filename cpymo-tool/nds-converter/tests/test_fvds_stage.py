"""FVDS frame/audio staging: callback ordering, timestamps, skip and faults.

Compiles the *production C source* with the actual container parser and
ADPCM decoder. Synthetic samples only, no licensed third-party player code.
"""
import shutil, subprocess, tempfile, unittest
from pathlib import Path
from test_fvds_inspect import sample

ROOT=Path(__file__).resolve().parents[3]
SRC=ROOT/"cpymo-backends/nds/source"
C_FILES=["fvds-stream-nds.c","fvds-adpcm-nds.c","fvds-stage-nds.c"]

DRIVER=r"""
#include "fvds-stage-nds.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(v) do{if(!(v))abort();}while(0)
struct state {
 unsigned begins,ends,chunks,video_bytes,audio_blocks,polls;
 uint64_t last_pts;
 int mode;
};
static int poll(void *context){
 struct state*s=context;s->polls++;
 /* Ensure the first completed frame stays fully complete. */
 if(s->mode==1 && s->begins>=2 && s->polls>20)return 1;
 return 0;
}
static int begin(void *context,uint32_t frame,uint64_t pts){
 struct state*s=context;
 REQUIRE(frame==s->begins);
 REQUIRE(pts==(uint64_t)frame*1000000/12);
 s->last_pts=pts;s->begins++;
 return 0;
}
static int compressed_video(void *context,const uint8_t *bytes,size_t count){
 struct state*s=context;
 REQUIRE(bytes && count && count<=512);
 if(s->mode==2)return -1;
 s->chunks++;s->video_bytes+=(unsigned)count;
 return 0;
}
static int end(void *context,uint32_t frame){
 struct state*s=context;
 REQUIRE(frame==s->ends);s->ends++;return 0;
}
static int pcm(void *context,const int16_t*bytes,size_t frames){
 struct state*s=context;
 REQUIRE(frames==256);
 for(size_t i=0;i<frames*2;i++)REQUIRE(bytes[i]==0);
 s->audio_blocks++;return 0;
}
int main(int argc,char**argv){
 REQUIRE(argc==3);
 nds_fvds_stream stream;
 REQUIRE(nds_fvds_open(&stream,argv[1])==0);
 nds_fvds_stage stage={0};
 struct state state={0};
 state.mode=atoi(argv[2]);
 nds_fvds_stage_callbacks callbacks={poll,begin,compressed_video,end,pcm};
 int status=0;
 while((status=nds_fvds_stage_step(&stream,&stage,&callbacks,&state))==0){}
 if(state.mode==0 || state.mode==5){
  REQUIRE(status==NDS_FVDS_STEP_EOF);
  REQUIRE(stage.completed_frames==3 && stage.audio_frames==47u*256u);
  REQUIRE(stage.key_pictures==1 && stage.predicted_pictures==(state.mode==5?1u:0u));
  REQUIRE(state.audio_blocks==47 && state.video_bytes==24);
  REQUIRE(state.begins==3 && state.ends==3);
  REQUIRE(state.last_pts==166666);
 }else if(state.mode==1){
  REQUIRE(status==NDS_FVDS_STEP_SKIPPED);
  REQUIRE(stage.completed_frames>=1 && stage.completed_frames<3);
  REQUIRE(stream.packet_open==0);
 }else if(state.mode==2){
  REQUIRE(status==NDS_FVDS_STEP_ERROR);
  REQUIRE(stage.completed_frames==0 && stream.packet_open==0);
 }else if(state.mode==3 || state.mode==4){
  REQUIRE(status==NDS_FVDS_STEP_ERROR);
  REQUIRE(stream.packet_open==0);
 }else REQUIRE(0);
 nds_fvds_close(&stream);
 return 0;
}
"""

@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class FVDSStageTests(unittest.TestCase):
    def test_callbacks_exit_and_truncated_audio(self):
        with tempfile.TemporaryDirectory() as td:
            d=Path(td);driver=d/"driver.c"
            driver.write_text(DRIVER,encoding="utf-8")
            binary=d/"driver"
            subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                            "-fsanitize=undefined","-I",str(SRC),
                            *(str(SRC/name) for name in C_FILES),
                            str(driver),"-o",str(binary)],check=True)
            source=d/"three-frames.fv";source.write_bytes(sample())
            for mode in range(3):
                subprocess.run([str(binary),str(source),str(mode)],
                               capture_output=True,text=True,check=True)
            damaged=bytearray(sample())
            # First packet begins at byte 36; 4-byte packed header + 8-byte
            # video. The first audio L-channel step index is at +2.
            damaged[36+4+8+2]=89
            source.write_bytes(damaged)
            subprocess.run([str(binary),str(source),"3"],
                           capture_output=True,text=True,check=True)
            bad_keyframe=bytearray(sample())
            # The indexed first video picture must have its I/P flag clear.
            bad_keyframe[36+4+1]|=0x80
            source.write_bytes(bad_keyframe)
            subprocess.run([str(binary),str(source),"4"],
                           capture_output=True,text=True,check=True)
            predicted=bytearray(sample())
            # The second packet is intentionally non-key. A P flag is valid
            # here, unlike in the first indexed I-picture.
            second=36+(4+8+16*264)
            predicted[second+4+1]|=0x80
            source.write_bytes(predicted)
            subprocess.run([str(binary),str(source),"5"],
                           capture_output=True,text=True,check=True)
