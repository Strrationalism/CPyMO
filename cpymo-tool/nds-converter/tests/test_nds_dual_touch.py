"""Native three-state DS touch routing regression.

Compiles the SAME dual-input-nds.h C used by the ARM9 input callback:
classic upper touch=A; classic lower true PyMO coordinates; 3DS-style
enhanced lower menu/choice hit-tests; stylus wake/touch lockout.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

SRC=Path(__file__).resolve().parents[3]/"cpymo-backends/nds/source"
CODE=r"""
#include "dual-input-nds.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#define CHECK(x) assert(x)
static nds_touch_sample project(unsigned mode,unsigned visible,unsigned ui,
                                unsigned choice,unsigned held,unsigned block,
                                unsigned key_a,int x,int y,
                                float scale,float lx,float ly){
 return nds_touch_project(mode,visible,ui,choice,held,block,key_a,
                          x,y,scale,lx,ly);
}
int main(void){
 /* Classic top uses the physically touched helper as NEXT, not x/y input.
    The auxiliary screen cannot accidentally select an invisible option. */
 nds_touch_sample s=project(0,0,1,1,1,0,0,144,100,.8f,0,0);
 CHECK(s.ok==1 && !s.mouse_button && s.x==0 && s.y==0);
 /* Classic bottom scales letterboxed 320x240 coordinates exactly. */
 s=project(1,0,0,0,1,0,0,128,96,.8f,0,0);
 CHECK(!s.ok && s.mouse_button && s.x==160 && s.y==120);
 /* Enhanced scene top + authored menu elements bottom. */
 s=project(2,1,1,0,1,0,0,128,96,.8f,0,0);
 CHECK(!s.ok && s.mouse_button && s.x==160 && s.y==120);
 /* Enhanced choices below use the same author-native hit targets. */
 s=project(2,1,0,1,1,0,0,128,96,.8f,0,0);
 CHECK(!s.ok && s.mouse_button && s.y==120);
 /* Enhance narrative only: tap anywhere to advance; no phantom mouse. */
 s=project(2,1,0,0,1,0,0,128,96,.8f,0,0);
 CHECK(s.ok && !s.mouse_button);
 /* Hidden sub LCD is NOT a usable interactive game screen. A wake-up
    contact is blocked by nds_aux_poll, regardless of its old coordinates. */
 s=project(2,0,1,1,1,1,0,128,96,.8f,0,0);
 CHECK(!s.ok && !s.mouse_button);
 /* L mode-cycle while a stylus is already held: blocked contact is
    ignored until release, no accidental on-release choice click. */
 s=project(1,0,1,1,1,1,0,128,96,.8f,0,0);
 CHECK(!s.ok && !s.mouse_button);
 /* Handheld A remains functional even when touch is blocked. */
 s=project(2,1,0,0,1,1,1,100,50,.8f,0,0);
 CHECK(s.ok==1 && !s.mouse_button);
 /* Nonzero letterbox offsets are removed in physical 256x192 pixels. */
 s=project(2,1,1,0,1,0,0,128,96,.5f,8.f,20.f);
 CHECK(s.mouse_button && fabsf(s.x-240.f)<.01f &&
       fabsf(s.y-152.f)<.01f);
 /* All absent input yields a completely inert neutral snapshot. */
 s=project(1,1,1,1,0,0,0,128,96,1.f,0,0);
 CHECK(!s.ok && !s.mouse_button);
 /* The same held stylus cannot become a fresh menu/choice click merely
    because an interpreter state changed during this contact. */
 nds_touch_role_tracker state={0};
 CHECK(nds_touch_role_step(&state,0,0)==0);
 CHECK(nds_touch_role_step(&state,0,1)==0); /* narrative begins */
 CHECK(nds_touch_role_step(&state,1,1)==2); /* narrative -> choice */
 CHECK(nds_touch_role_step(&state,1,1)==0); /* remains blocked by caller */
 CHECK(nds_touch_role_step(&state,1,0)==0); /* lift clears global block */
 CHECK(nds_touch_role_step(&state,1,1)==0); /* fresh menu tap allowed */
 CHECK(nds_touch_role_step(&state,0,0)==1); /* UI -> story on release */
 CHECK(nds_touch_role_step(&state,0,1)==0); /* next story tap allowed */
 CHECK(nds_touch_role_step(&state,1,0)==1); /* pen lifted on role edge */
 CHECK(nds_touch_role_step(&state,0,0)==0); /* no contact, no block */
 CHECK(nds_touch_role_step(&state,1,1)==0); /* fresh tap after UI opens */
 CHECK(nds_touch_role_step(NULL,0,1)==0);
 /* Real modes 0 and 1 remain invariant across UI/selection changes. */
 for(unsigned mode=0;mode<2;mode++){
  unsigned expected=mode==1;
  for(unsigned ui=0;ui<2;ui++)
   for(unsigned choice=0;choice<2;choice++)
    CHECK(nds_touch_uses_coordinates(mode,1,ui,choice)==expected);
 }
 return 0;
}
"""

@unittest.skipUnless(shutil.which("gcc"),"Host GCC unavailable")
class NDSDualTouchTests(unittest.TestCase):
 def test_real_three_mode_projection_and_touch_block(self):
  with tempfile.TemporaryDirectory() as folder:
   root=Path(folder)
   src=root/"check.c";src.write_text(CODE,encoding="utf-8")
   exe=root/"check"
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-fsanitize=undefined","-I",str(SRC),str(src),
                   "-lm","-o",str(exe)],check=True,
                  capture_output=True,text=True)
   subprocess.run([str(exe)],check=True,capture_output=True,text=True)
