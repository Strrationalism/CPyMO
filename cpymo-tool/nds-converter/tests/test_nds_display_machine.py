"""Compile and run production dual-screen-nds.h display-state machine with
host stubs, including actual saved mode bytes and stylus ghost-click guard.
No simulated UI image is produced; this tests real control state only.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

SRC=Path(__file__).resolve().parents[3]/"cpymo-backends/nds/source"
C_PROGRAM=r"""
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
typedef uint64_t u64;
#define TICK_FREQ 1000u
static u64 fake_ticks;
static u64 tickGetCount(void){return fake_ticks;}
enum cpymo_backend_image_draw_type{
 cpymo_backend_image_draw_type_bg=0,
 cpymo_backend_image_draw_type_text_say,
 cpymo_backend_image_draw_type_text_say_textbox,
 cpymo_backend_image_draw_type_ui_bg,
 cpymo_backend_image_draw_type_ui_element,
 cpymo_backend_image_draw_type_ui_element_bg,
 cpymo_backend_image_draw_type_sel_img
};
typedef struct {
 struct { int imagesize_w,imagesize_h,nameboxorg_y; }gameconfig;
 struct {void *msgbox,*name,*namebox; int msgbox_h,msgbox_w;}say;
 struct {void *selections;}select_img;
 struct {const char *gamedir;}assetloader;
 struct {bool ok,mouse_button,mouse_position_useable;}input;
 bool ignore_next_mouse_button_flag;
} cpymo_engine;
static cpymo_engine engine;
static bool nds_playing=true,error_ui_active=false,nds_aux_override=false;
static bool nds_touch_blocked=false,screens_swapped=false,menu_enabled=false,aux_hidden=false;
static volatile unsigned presentation_state[9];
static int keys=0,remap=0,refresh=0,redraw=0,ignored=0,vblanks=0;
#define KEY_TOUCH 1
#define KEY_A 2
static int keysHeld(void){return keys;}
static void lcdMainOnTop(void){remap=0;}
static void lcdSwap(void){remap=!remap;}
static void swiWaitForVBlank(void){vblanks++;}
static void nds_aux_refresh(void){refresh++;}
static bool nds_aux_hidden(void){return aux_hidden;}
static bool cpymo_ui_enabled(const cpymo_engine *e){(void)e;return menu_enabled;}
static float cpymo_gameconfig_font_size(const void *cfg){(void)cfg;return 20.f;}
static void cpymo_engine_request_redraw(cpymo_engine *e){(void)e;redraw++;}
static void cpymo_input_ignore_next_mouse_button_event(cpymo_engine *e){
 ignored++;e->ignore_next_mouse_button_flag=e->input.mouse_button;
}
static float display_scale(void){return .8f;}
static float display_y(void){return 0.f;}
#include "save-dir-nds.h"
#include "dual-screen-nds.h"

static void verify_not_saved(const char *dir){
 char p[512];
 snprintf(p,sizeof p,"%s/save/nds-dualscreen-mode.txt",dir);
 FILE *f=fopen(p,"rb");
 assert(!f);
}
static void verify_saved(const char *dir,const char *want){
 char p[512],actual[80]={0};
 snprintf(p,sizeof p,"%s/save/nds-dualscreen-mode.txt",dir);
 FILE *f=fopen(p,"rb");assert(f);
 size_t n=fread(actual,1,sizeof actual-1,f);
 fclose(f);
 assert(n==strlen(want));
 assert(!strcmp(actual,want));
}
int main(int argc,char**argv){
 assert(argc==2);
 engine.assetloader.gamedir=argv[1];
 engine.gameconfig.imagesize_w=320;
 engine.gameconfig.imagesize_h=240;
 engine.say.msgbox_w=320;
 engine.say.msgbox_h=60;
 nds_dual_load(argv[1]);
 nds_display_start();
 assert(nds_display_mode==0 && !screens_swapped && remap==0);
 assert(nds_dual_accept(cpymo_backend_image_draw_type_text_say));
 /* Hold stylus in a lower touch menu and press L; its eventual release
    must not get interpreted as the next mode's tap or selection. */
 keys=KEY_TOUCH;
 engine.input.mouse_button=true;
 nds_display_cycle();
 assert(nds_display_mode==1 && screens_swapped && remap==1);
 assert(nds_touch_blocked && ignored==1 && engine.ignore_next_mouse_button_flag);
 assert(!engine.input.mouse_button && !engine.input.mouse_position_useable);
 assert(refresh==1 && redraw==1);
 assert(nds_dual_save_pending && nds_dual_state[4]==0);
 verify_not_saved(argv[1]);
 /* If the user cycles quickly, each L resets the FAT-write cooldown. */
 fake_ticks=500;nds_dual_poll_mode();
 verify_not_saved(argv[1]);
 /* Releasing stylus lets the real input poll clear touch_blocked. */
 keys=0;nds_touch_blocked=false;
 nds_display_cycle();
 /* The LCD remains mapped to classic-lower until both new framebuffers
    have been published, then commits the switch at VBlank. */
 assert(nds_display_mode==2 && nds_dual_mode && screens_swapped && remap==1);
 assert(nds_display_swap_pending && vblanks==0);
 nds_display_commit_pending_swap();
 assert(!screens_swapped && remap==0 && !nds_display_swap_pending && vblanks==1);
 assert(refresh==2 && redraw==2);
 fake_ticks=1200;nds_dual_poll_mode();
 verify_not_saved(argv[1]); /* new deadline is 1250 ticks */
 fake_ticks=1250;nds_dual_poll_mode();
 verify_saved(argv[1],"NDSDISPLAY2 2\n");
 assert(nds_dual_state[4]==1 && !nds_dual_save_pending);
 menu_enabled=false;nds_dual_bottom_pass=false;
 /* Initial title/boot selection is an unsplit upper-LCD scene. */
 assert(!nds_dual_title_allow_split(&nds_title_gate));
 assert(nds_dual_accept(cpymo_backend_image_draw_type_text_say));
 assert(!nds_dual_title_observe(&nds_title_gate,0,0,1)); /* title choice */
 assert(nds_dual_title_observe(&nds_title_gate,1,0,0)); /* story begins */
 assert(nds_dual_title_allow_split(&nds_title_gate));
 assert(nds_dual_accept(cpymo_backend_image_draw_type_bg));
 assert(!nds_dual_accept(cpymo_backend_image_draw_type_text_say));
 nds_dual_bottom_pass=true;
 assert(nds_dual_accept(cpymo_backend_image_draw_type_text_say));
 assert(nds_dual_say_offset() < 0.f);
 assert(!nds_dual_accept(cpymo_backend_image_draw_type_bg));
 /* Narrative selection images MUST route to the touch LCD. */
 engine.select_img.selections=(void*)1;
 assert(nds_dual_accept(cpymo_backend_image_draw_type_sel_img));
 menu_enabled=true;
 assert(nds_dual_accept(cpymo_backend_image_draw_type_ui_element));
 assert(!nds_dual_accept(cpymo_backend_image_draw_type_ui_bg));
 nds_dual_bottom_pass=false;
 assert(nds_dual_accept(cpymo_backend_image_draw_type_ui_bg));
 assert(!nds_dual_accept(cpymo_backend_image_draw_type_ui_element));
 menu_enabled=false;
 /* Three-state return, new game reloads its own preference. */
 nds_display_cycle();
 assert(nds_display_mode==0 && !nds_dual_mode && remap==0);
 verify_saved(argv[1],"NDSDISPLAY2 2\n"); /* not yet written */
 fake_ticks=2010;nds_dual_poll_mode();
 verify_saved(argv[1],"NDSDISPLAY2 0\n");
 assert(nds_dual_state[4]==2);
 /* Reproduce real ghost-A: classic-upper turns held TOUCH into OK, not
    mouse_button. An unsanitized old OK generates JUST_RELEASED in the
    subsequent core update, advancing a story from a mode-only L press. */
 keys=KEY_TOUCH;
 engine.input.ok=true;
 engine.input.mouse_button=false;
 engine.input.mouse_position_useable=false;
 nds_display_cycle();
 assert(nds_display_mode==1 && nds_touch_blocked);
 assert(!engine.input.ok && !engine.input.mouse_button);
 assert(!engine.input.mouse_position_useable);
 verify_saved(argv[1],"NDSDISPLAY2 0\n"); /* still coalescing */
 fake_ticks=2250;nds_dual_poll_mode();
 assert(nds_dual_state[4]==2);
 /* Physical A is preserved while the obsolete stylus input is discarded. */
 keys=KEY_TOUCH|KEY_A;
 engine.input.ok=true;
 engine.input.mouse_button=true;
 engine.input.mouse_position_useable=true;
 nds_display_cycle();
 assert(nds_display_mode==2 && engine.input.ok);
 assert(nds_display_swap_pending && remap==1);
 assert(!engine.input.mouse_button && !engine.input.mouse_position_useable);
 keys=0;
 /* Exit must persist even before the 750ms threshold; never lose last
    selected mode because the story completed quickly. */
 assert(nds_dual_save_pending && nds_dual_state[4]==2);
 nds_aux_override=true;
 nds_display_restore_launcher();
 assert(nds_dual_state[4]==3 && !nds_dual_save_pending);
 verify_saved(argv[1],"NDSDISPLAY2 2\n");
 assert(!nds_aux_override && !nds_dual_bottom_pass && remap==0);
 assert(refresh==5 && redraw==5);
 nds_dual_load(argv[1]);
 assert(nds_display_mode==2 && !nds_dual_save_pending);
 assert(!nds_dual_title_allow_split(&nds_title_gate)); /* next launch title */
 return 0;
}
"""
@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class NDSDisplayMachineTests(unittest.TestCase):
 def test_live_header_mode_cycle_stylus_and_saved_state(self):
  with tempfile.TemporaryDirectory() as td:
   base=Path(td)
   game=base/"game";game.mkdir(parents=True)
   src=base/"mode.c";src.write_text(C_PROGRAM,encoding="utf-8")
   exe=base/"mode"
   result=subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                          "-fsanitize=undefined","-I",str(SRC),str(src),
                          "-o",str(exe)],capture_output=True,text=True)
   self.assertEqual(result.returncode,0,result.stderr)
   subprocess.run([str(exe),str(game)],check=True,
                  capture_output=True,text=True)
