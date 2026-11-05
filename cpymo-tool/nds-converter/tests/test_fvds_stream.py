"""Host executable regression for the ORIGINAL FVDS packet reader.

Compiles the same C source used in ARM9 with the native host compiler.
Only synthetic 3-frame samples from test_fvds_inspect.py are used.
No reference FastVideoDS decoder source or commercial game assets.
"""
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path
from test_fvds_inspect import sample

ROOT=Path(__file__).resolve().parents[3]
SOURCE=ROOT/"cpymo-backends/nds/source/fvds-stream-nds.c"
INCLUDE=ROOT/"cpymo-backends/nds/source"

RUNNER=r"""
#include "fvds-stream-nds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static void ck(int yes) { if(!yes)abort(); }
int main(int argc,char **argv){
 if(argc!=3)return 2;
 nds_fvds_stream s;nds_fvds_packet p;
 int result=nds_fvds_open(&s,argv[2]);
 if(!strcmp(argv[1],"bad-header")){ck(result!=0);return 0;}
 if(!strcmp(argv[1],"bad-packet")){ck(result==0);ck(nds_fvds_next(&s,&p)==-3);nds_fvds_close(&s);return 0;}
 ck(result==0);ck(s.frame_count==3);ck(s.width==256);ck(s.keyframe_count==1||s.keyframe_count==2);
 uint8_t chunk[1007];
 for(unsigned frame=0;frame<3;frame++){
  int rc=nds_fvds_next(&s,&p);
  if(!strcmp(argv[1],"bad-index")&&frame==2){
   ck(rc==-6);nds_fvds_close(&s);return 0;
  }
  ck(rc==0);ck(p.frame==frame);
  ck(p.video_bytes==8);
  for(unsigned remain=p.video_bytes;remain;){
   size_t n=nds_fvds_read_video(&s,chunk,sizeof(chunk));
   ck(n>0&&n<=remain);remain-=n;
  }
  for(unsigned remain=p.audio_bytes;remain;){
   size_t n=nds_fvds_read_audio(&s,chunk,sizeof(chunk));
   ck(n>0&&n<=remain);remain-=n;
  }
  ck(nds_fvds_finish(&s)==0);
 }
 result=nds_fvds_next(&s,&p);
 ck(result==(!strcmp(argv[1],"tail")?-5:1));
 ck(nds_fvds_seek_keyframe(&s,0)==0);
 ck(nds_fvds_next(&s,&p)==0 && p.frame==0);
 ck(nds_fvds_read_audio(&s,chunk,sizeof(chunk))==0);
 ck(nds_fvds_finish(&s)==0);
 nds_fvds_close(&s);return 0;
}
"""

@unittest.skipUnless(shutil.which("gcc"),"C compiler unavailable")
class FVDSStreamTests(unittest.TestCase):
    @staticmethod
    def two_keyframe_sample(invalid=False):
        original=sample()
        head=bytearray(original[:28])
        struct.pack_into("<I",head,24,2)
        # Original first/second packets contain 16 audio block pairs.
        second_start=44+2*(4+8+16*264)
        key2=struct.pack("<II",2,second_start+(4 if invalid else 0))
        return bytes(head)+struct.pack("<II",0,44)+key2+original[36:]

    def test_synthetic_packet_stream_and_rejection(self):
        with tempfile.TemporaryDirectory() as d:
            d=Path(d)
            (d/"runner.c").write_text(RUNNER,encoding="utf-8")
            exe=d/"runner"
            subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                            "-fsanitize=undefined","-I",str(INCLUDE),
                            str(SOURCE),str(d/"runner.c"),"-o",str(exe)],check=True)
            for mode,raw in [("valid",sample()),("bad-header",sample(bad_key=True)),
                             ("tail",sample(tail=True)),("bad-packet",sample(bad_video=True)),
                             ("valid",self.two_keyframe_sample()),
                             ("bad-index",self.two_keyframe_sample(invalid=True))]:
                path=d/(mode+str(len(raw))+".fv")
                path.write_bytes(raw)
                subprocess.run([str(exe),mode,str(path)],check=True,
                               capture_output=True,text=True)

if __name__=="__main__":
    unittest.main()
