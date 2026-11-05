"""End-to-end packet + audio decode contract using the production NDS C modules.
The fixture is synthetic silent FVDS with 3 frames; nothing from commercial media
or unlicensed player sources is committed.
"""
from pathlib import Path
import shutil,subprocess,tempfile,unittest
from test_fvds_inspect import sample

ROOT=Path(__file__).resolve().parents[3]
INC=ROOT/"cpymo-backends/nds/source"
C=[INC/"fvds-stream-nds.c",INC/"fvds-adpcm-nds.c"]
RUNNER=r"""
#include "fvds-stream-nds.h"
#include "fvds-adpcm-nds.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static void ck(int condition){if(!condition)abort();}
int main(int argc,char **argv){
 ck(argc==2);
 nds_fvds_stream stream;nds_fvds_packet packet;
 ck(nds_fvds_open(&stream,argv[1])==0);
 uint8_t compressed[264],video[512];int16_t decoded[512];
 unsigned frames=0,pairs=0,readbytes=0;
 while(nds_fvds_next(&stream,&packet)==0){
  ck(packet.frame==frames);
  unsigned vremain=packet.video_bytes;
  while(vremain){
   size_t got=nds_fvds_read_video(&stream,video,sizeof video);
   ck(got&&got<=vremain);vremain-=got;readbytes+=got;
  }
  ck(nds_fvds_read_audio(&stream,compressed,264)==264);
  ck(nds_fvds_adpcm_pair(compressed,264,decoded,512)==256);
  for(unsigned i=0;i<512;i++)ck(decoded[i]==0);
  pairs++;readbytes+=264;
  for(unsigned b=1;b<packet.audio_blocks;b++){
   ck(nds_fvds_read_audio(&stream,compressed,264)==264);
   ck(nds_fvds_adpcm_pair(compressed,264,decoded,512)==256);
   for(unsigned i=0;i<512;i++)ck(decoded[i]==0);
   pairs++;readbytes+=264;
  }
  ck(nds_fvds_finish(&stream)==0);
  frames++;
 }
 ck(frames==3&&pairs==47&&readbytes==24+47*264);
 ck(nds_fvds_seek_keyframe(&stream,0)==0);
 ck(nds_fvds_next(&stream,&packet)==0&&packet.frame==0);
 ck(nds_fvds_finish(&stream)==0);
 nds_fvds_close(&stream);
 return 0;
}
"""

@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class FVDSAudioPipelineTests(unittest.TestCase):
    def test_synthetic_full_pipeline(self):
        with tempfile.TemporaryDirectory() as td:
            d=Path(td)
            (d/"runner.c").write_text(RUNNER)
            binary=d/"runner"
            subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                            "-fsanitize=undefined","-I",str(INC),
                            *(str(p) for p in C),str(d/"runner.c"),"-o",str(binary)],check=True)
            fixture=d/"three-frames.fv";fixture.write_bytes(sample())
            subprocess.run([str(binary),str(fixture)],check=True,capture_output=True,text=True)

if __name__=="__main__":
 unittest.main()
