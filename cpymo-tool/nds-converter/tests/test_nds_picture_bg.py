"""ROM-owned CPyMO 1.2 enhanced touch-screen artwork regression.

No per-card external bitmap is required: immutable bytes are patched into the
ROM's fixed-size ARM9 placeholder. Verify safe unpatched fallback, low-contrast
UI RGB555 dimming, and deterministic no-relocation ROM injection.
"""
from pathlib import Path
import importlib.util
import shutil
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SRC = ROOT / "cpymo-backends/nds/source"
PATCHER = ROOT / "cpymo-backends/nds/tools/patch-rom-bg.py"
spec = importlib.util.spec_from_file_location("patch_rom_bg", PATCHER)
patcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patcher)

C_PROGRAM = r"""
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "nds-picture-bg.h"
int main(void) {
 uint16_t pixels[NDS_PICTURE_PIXELS] = {0};
 /* An unpatched developer build should never display staging pixels. */
 assert(nds_picture_get() == 0);
 assert(!nds_picture_draw(pixels, false));
 assert(!nds_picture_draw(pixels, true));
 assert(nds_picture_dim_color(0x8000) == 0x8000);
 assert(nds_picture_dim_color(0xffff) == (0x8000 | 23 | (23<<5) | (23<<10)));
 assert(nds_picture_dim_color(0x801f) == (0x8000 | 23));
 return 0;
}
"""

@unittest.skipUnless(shutil.which("gcc"), "host GCC unavailable")
class EmbeddedPictureTests(unittest.TestCase):
 def test_unpatched_fallback_and_rgb555_dimming(self):
  with tempfile.TemporaryDirectory() as folder:
   folder=Path(folder)
   src=folder/"test.c"
   src.write_text(C_PROGRAM,encoding="utf8")
   exe=folder/"test"
   cmd=["gcc","-std=gnu11","-O2","-Wall","-Wextra","-Werror",
        "-fsanitize=undefined","-I",str(SRC),str(src),"-o",str(exe)]
   run=subprocess.run(cmd,capture_output=True,text=True)
   self.assertEqual(run.returncode,0,run.stderr)
   subprocess.run([str(exe)],capture_output=True,text=True,check=True)

 def test_inplace_patch_in_rom_and_reject_corruption(self):
  with tempfile.TemporaryDirectory() as folder:
   folder=Path(folder)
   image=(b"\x01\x80")*(256*192)
   art=folder/"art.rgb555"
   art.write_bytes(patcher.ART_MAGIC+image)
   mark=patcher.WAIT+patcher.PLACEHOLDER
   raw=bytearray(0x200+len(mark)+64)
   struct.pack_into("<I",raw,0x20,0x200)
   struct.pack_into("<I",raw,0x2c,len(mark)+64)
   raw[0x200:0x200+len(mark)]=mark
   rom=folder/"input.nds";rom.write_bytes(raw)
   out=folder/"patched.nds"
   report=patcher.embed(rom,art,out)
   patched=out.read_bytes()
   self.assertTrue(report["single_nds"])
   self.assertEqual(len(patched),len(raw))
   self.assertEqual(patched[:0x200],raw[:0x200])
   self.assertEqual(patched[0x200:0x200+16],patcher.READY)
   self.assertEqual(patched[0x210:0x210+len(image)],image)
   self.assertEqual(patched[-64:],raw[-64:])
   with self.assertRaises(ValueError):
    patcher.embed(out,art,folder/"second.nds")
   art.write_bytes(patcher.ART_MAGIC+(b"\x00\x00")*(256*192))
   with self.assertRaises(ValueError):
    patcher.embed(rom,art,folder/"invalid.nds")

 def test_movie_playback_clears_previous_lower_screen_dialogue_once(self):
  # The video player bypasses ordinary engine draws while pushing frames
  # straight to top LCD VRAM. It must redraw the lower LCD without say text.
  player=(SRC/"movie-lz4-r7.h").read_text(encoding="utf8")
  helper=player[player.index("static void nds_movie_prepare_lower_screen(void)"):]
  helper=helper[:helper.index("\n}")]
  self.assertIn("if(!nds_dual_active() || !launcher_sub)return;",helper)
  self.assertIn("nds_picture_draw(framebuffer,false)",helper)
  self.assertIn("nds_dual_canvas_draw_yield(framebuffer,false,false,",helper)
  self.assertIn("dmaCopy(framebuffer,launcher_sub,sizeof framebuffer)",helper)
  self.assertIn("if(failed)goto done;",player)
  self.assertLess(player.index("if(failed)goto done;"),
                  player.index("nds_movie_prepare_lower_screen();"))
  self.assertLess(player.index("nds_movie_prepare_lower_screen();"),
                  player.index("memset(framebuffer,0,sizeof framebuffer);movie_state[1]=1;"))
  self.assertEqual(player.count("nds_movie_prepare_lower_screen();"),1)
  self.assertIn("cpymo_engine_request_redraw(e);return CPYMO_ERR_SUCC;",player)

 def test_ui_and_confirmation_both_select_approved_art(self):
  source=(SRC/"main.c").read_text(encoding="utf8")
  self.assertIn("nds_picture_draw(framebuffer,interactive)",source)
  self.assertIn("nds_dual_lower_background(cpymo_ui_enabled(&engine),nds_dual_has_choice())",source)
  self.assertIn("cpymo_engine_draw(&engine)",source)

