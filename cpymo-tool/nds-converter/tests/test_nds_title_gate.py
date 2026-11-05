"""Hardware-neutral regression: startup/title stays fully upper until dialogue.

This is a deliberately limited opening-scene state machine. It does not
attempt to identify game-specific MENU or START filenames, modify scripts,
or remap touch/gamepad controls. The first true narrative say takes over.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[3]
SOURCE=ROOT/'cpymo-backends/nds/source'
PROGRAM=r"""
#include "dual-title-nds.h"
#include <stdlib.h>
#define CHECK(x) do{if(!(x))abort();}while(0)
int main(void){
 nds_dual_title_gate gate={0};
 nds_dual_title_reset(&gate);
 CHECK(!nds_dual_title_allow_split(&gate));
 CHECK(!nds_dual_title_observe(&gate,0,0,0)); /* boot splash */
 CHECK(!nds_dual_title_observe(&gate,0,0,1)); /* title selection */
 CHECK(!nds_dual_title_observe(&gate,0,1,0)); /* menu/settings */
 CHECK(!nds_dual_title_observe(&gate,1,1,0)); /* modal UI */
 CHECK(!nds_dual_title_observe(&gate,1,0,1)); /* choice before story */
 CHECK(!nds_dual_title_allow_split(&gate));
 CHECK(nds_dual_title_observe(&gate,1,0,0)); /* first real story say */
 CHECK(nds_dual_title_allow_split(&gate));
 CHECK(!nds_dual_title_observe(&gate,0,1,0)); /* later X menu */
 CHECK(nds_dual_title_allow_split(&gate));
 CHECK(!nds_dual_title_observe(&gate,1,0,0)); /* not twice */
 nds_dual_title_reset(&gate); /* a newly launched or restarted game */
 CHECK(!nds_dual_title_allow_split(&gate));
 CHECK(!nds_dual_title_observe(&gate,0,0,1)); /* title selection again */
 CHECK(nds_dual_title_observe(&gate,1,0,0)); /* next story */
 CHECK(nds_dual_title_allow_split(&gate));
 return 0;
}
"""


@unittest.skipUnless(shutil.which('gcc'),"C host compiler unavailable")
class NDSTitleGateTests(unittest.TestCase):
    def test_startup_then_story_state_machine(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            test=root/'test.c'
            test.write_text(PROGRAM,encoding='utf-8')
            exe=root/'test'
            subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror',
                            '-fsanitize=undefined','-I',str(SOURCE),
                            str(test),'-o',str(exe)],check=True,capture_output=True,text=True)
            subprocess.run([str(exe)],check=True,capture_output=True,text=True)

    def test_integration_keeps_touch_and_lcd_semantics(self):
        dual=(SOURCE/'dual-screen-nds.h').read_text(encoding='utf-8')
        backend=(SOURCE/'main.c').read_text(encoding='utf-8')
        self.assertIn('nds_dual_title_reset(&nds_title_gate);',dual)
        self.assertIn('nds_dual_title_allow_split(&nds_title_gate)',dual)
        self.assertIn('nds_dual_title_observe(&nds_title_gate',backend)
        self.assertIn('engine.say.active,cpymo_ui_enabled(&engine)',backend)
        self.assertIn('nds_display_mode==2',backend)
        self.assertIn('nds_aux_refresh();',backend)
        self.assertIn('nds_touch_project(nds_display_mode,',backend)
        self.assertIn('nds_dual_has_choice()',dual)
        self.assertIn('if(next==1){lcdSwap();screens_swapped=true;}',dual)


if __name__=='__main__':unittest.main()
