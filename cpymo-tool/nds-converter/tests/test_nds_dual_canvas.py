"""Execute the exact NDS enhanced RGB555 canvas C source on host.
Checks exact RGB555 background gradients, star field, low-contrast cloud
horizons, interactive suppression and six audio yield calls per pass.
"""
from pathlib import Path
import shutil,subprocess,tempfile,unittest

ROOT=Path(__file__).resolve().parents[3]
SOURCE=ROOT/"cpymo-backends/nds/source"
PROGRAM=r"""
#include "dual-canvas-nds.h"
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x))abort();}while(0)
static uint16_t source_a[256*192],source_b[256*192];
static int ticks;
static void audio_tick(void *unused){CHECK(unused==NULL);ticks++;}
int main(void){
 const uint16_t bg=nds_dual_canvas_color(5,9,13);
 const uint16_t dark=nds_dual_canvas_color(2,4,7);
 const uint16_t bright=nds_dual_canvas_color(12,18,23);
 for(int interactive=0;interactive<=1;interactive++){
  for(int dialogue=0;dialogue<=1;dialogue++){
   ticks=0;
   nds_dual_canvas_draw(source_a,interactive,dialogue);
   nds_dual_canvas_draw_yield(source_b,interactive,dialogue,
                              audio_tick,NULL);
   CHECK(ticks==6);  /* scanlines y=0,32,64,96,128,160 */
   CHECK(!memcmp(source_a,source_b,sizeof source_a));
   CHECK(source_a[1*256+0]==nds_dual_canvas_row(1));
   CHECK(source_a[0]==bg);
   CHECK(source_a[191*256+0]==(interactive?dark:nds_dual_canvas_cloud(191,1)));
   CHECK(source_a[7*256+15]==bright);
   /* Artistic sky/horizon only appears when the canvas is not occupied by
      a menu or story choice; original dialogue pixels are drawn on top. */
   CHECK(source_a[155*256+2]==(interactive?nds_dual_canvas_row(155):nds_dual_canvas_cloud(155,0)));
   CHECK(source_a[162*256+20]==(interactive?nds_dual_canvas_row(162):nds_dual_canvas_cloud(162,0)));
   CHECK(source_a[170*256+2]==(interactive?nds_dual_canvas_row(170):nds_dual_canvas_cloud(170,0)));
   CHECK(source_a[191*256+200]==(interactive?dark:nds_dual_canvas_cloud(191,1)));
   CHECK(source_a[125*256+132]==(interactive?nds_dual_canvas_row(125):
       nds_dual_canvas_color(9,14,19)));
   CHECK(source_a[108*256+206]==(interactive?nds_dual_canvas_row(108):
       nds_dual_canvas_color(9,14,19)));
   const uint16_t base=nds_dual_canvas_row(70);
   const uint16_t edge=nds_dual_canvas_color(
       (base&31u)+1u,((base>>5)&31u)+2u,((base>>10)&31u)+2u);
   CHECK(source_a[70*256+7]==(interactive?base:edge));
   CHECK(source_a[70*256+248]==(interactive?base:edge));
   CHECK(source_a[181*256+18]==(interactive?nds_dual_canvas_row(181):
          nds_dual_canvas_color(6,9,13)));
  }
 }
 return 0;
}
"""
@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class NDSDualCanvasTests(unittest.TestCase):
 def test_backdrop_and_cooperative_audio_cadence(self):
  with tempfile.TemporaryDirectory() as folder:
   d=Path(folder);code=d/"test.c";code.write_text(PROGRAM,encoding="utf-8")
   exe=d/"test"
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-fsanitize=undefined","-I",str(SOURCE),
                   str(code),"-o",str(exe)],
                  capture_output=True,text=True,check=True)
   subprocess.run([str(exe)],capture_output=True,text=True,check=True)
