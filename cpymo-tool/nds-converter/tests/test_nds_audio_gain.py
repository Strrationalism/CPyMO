"""v1.2.1 test3: bounded master gain, limiter and all audio output paths."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

SOURCE=Path(__file__).resolve().parents[3]/"cpymo-backends/nds/source"
PROGRAM=r"""
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "audio-gain-nds.h"
int main(void){
 NdsAudioGain g;
 int16_t out[64];int32_t raw[64],saved[64];
 nds_audio_gain_reset(&g);
 assert(g.gain_q12==NDS_AUDIO_GAIN_Q12_TARGET);
 for(unsigned i=0;i<64;i++)raw[i]=(i&1)?-8000:8000;
 nds_audio_gain_write(out,raw,64,&g);
 assert(out[0]>=9480&&out[0]<=9530);
 assert(out[1]<=-9480&&out[1]>=-9530);
 assert(g.limited_blocks==0&&g.samples==64);
 nds_audio_gain_reset(&g);
 for(unsigned i=0;i<64;i++)raw[i]=(i&1)?-32768:32767;
 nds_audio_gain_write(out,raw,64,&g);
 assert(g.limited_blocks==1&&g.preexisting_over_blocks==1);
 for(unsigned i=0;i<64;i++)assert(out[i]<=32760&&out[i]>=-32760);
 nds_audio_gain_reset(&g);
 for(unsigned i=0;i<64;i++)raw[i]=(i&1)?-98304:98301;
 nds_audio_gain_write(out,raw,64,&g);
 assert(g.preexisting_over_blocks==1&&g.max_input_peak==98304);
 assert(out[0]>30000&&out[1]<-30000);
 for(unsigned i=0;i<64;i++)assert(out[i]<=32760&&out[i]>=-32760);
 int previous=g.gain_q12;
 memset(raw,0,sizeof raw);
 nds_audio_gain_write(out,raw,64,&g);
 assert(g.gain_q12>previous&&g.gain_q12<NDS_AUDIO_GAIN_Q12_TARGET);
 for(unsigned i=0;i<500;i++)nds_audio_gain_write(out,raw,64,&g);
 assert(g.gain_q12==NDS_AUDIO_GAIN_Q12_TARGET);
 nds_audio_gain_reset(&g);
 for(unsigned i=0;i<64;i++)raw[i]=14000;
 memcpy(saved,raw,sizeof raw);
 nds_audio_gain_write(out,raw,64,&g);
 assert(memcmp(raw,saved,sizeof raw)==0&&out[0]>14000);
 return 0;
}
"""

class NDSAudioGainTests(unittest.TestCase):
 @unittest.skipUnless(shutil.which("gcc"),"host compiler unavailable")
 def test_integer_headroom_and_no_digital_clipping(self):
  with tempfile.TemporaryDirectory() as td:
   d=Path(td);(d/"test.c").write_text(PROGRAM,encoding="utf8")
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-fsanitize=undefined","-I",str(SOURCE),
                   str(d/"test.c"),"-o",str(d/"test")],check=True,capture_output=True,text=True)
   subprocess.run([str(d/"test")],check=True,capture_output=True,text=True)
 def test_pcm_mp3_movie_paths_share_peak_limiter(self):
  audio=(SOURCE/"audio-nds.c").read_text(encoding="utf8")
  self.assertEqual(audio.count("nds_audio_gain_write("),3)
  self.assertIn("nds_audio_gain_reset(&nds_gain)",audio)
  self.assertIn("int32_t mixed[64]={0}",audio)
  self.assertIn("int32_t raw[64]",audio)
  self.assertIn("int32_t mixed[64];",audio)
  video=audio.split("if(movie_audio){",1)[1].split("bool pcm_only=",1)[0]
  self.assertLess(video.index("nar_checksum("),video.index("nds_audio_gain_write("))
  self.assertIn("if(got<length)memset(out+got,0,(length-got)*2)",video)

if __name__=="__main__":
 unittest.main()
