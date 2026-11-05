"""Pure-C 3DS-inspired NDS dialogue geometry tests.

Use the actual production layout header.  Target the *physical* 256x192
touch LCD independently of original PyMO logical scene dimensions.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[3]
SRC=ROOT/"cpymo-backends/nds/source"

PROGRAM=r"""
#include "dual-layout-nds.h"
#include <assert.h>
#include <math.h>

static void test_placement(float logical_h,float msg_h,float font,
                           float off,int name,float scale,float letterbox){
 float delta=nds_dual_dialogue_shift(logical_h,msg_h,font,off,name,scale,letterbox);
 float top=logical_h-msg_h-(name?(font*1.4f+off):0.f);
 float position=(top+delta)*scale+letterbox;
 float bottom=(logical_h+delta)*scale+letterbox;
 assert(isfinite(delta));
 assert(position>=12.9f && position<=15.1f);
 assert(bottom<=185.1f);
}
int main(void){
 test_placement(360.f,90.f,26.f,0.f,1,192.f/360.f,0.f);
 test_placement(240.f,60.f,20.f,0.f,1,192.f/240.f,0.f);
 test_placement(360.f,90.f,26.f,18.f,1,192.f/360.f,0.f);
 test_placement(360.f,90.f,26.f,0.f,0,192.f/360.f,0.f);
 /* Letterbox y differs when logical aspect ratio exceeds 4:3. */
 test_placement(200.f,50.f,16.f,0.f,1,256.f/480.f,
                (192.f-200.f*(256.f/480.f))/2);
 /* An overlarge game-authored textbox must not be shifted into clipping. */
 assert(nds_dual_dialogue_shift(192.f,190.f,14.f,0.f,1,1.f,0.f)==0.f);
 assert(nds_dual_dialogue_shift(360.f,90.f,26.f,0.f,1,0.f,0.f)==0.f);
 return 0;
}
"""
@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class NDSDualLayoutTests(unittest.TestCase):
    def test_dialogue_geometry_and_preserve_oversized(self):
        with tempfile.TemporaryDirectory() as folder:
            folder=Path(folder)
            source=folder/"layout.c"
            source.write_text(PROGRAM,encoding="utf-8")
            binary=folder/"layout"
            subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                            "-fsanitize=undefined","-I",str(SRC),
                            str(source),"-lm","-o",str(binary)],
                           check=True,capture_output=True,text=True)
            subprocess.run([str(binary)],capture_output=True,
                           text=True,check=True)

if __name__=="__main__":
    unittest.main()
