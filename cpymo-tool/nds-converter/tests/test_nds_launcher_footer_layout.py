"""Pixel geometry and action-hint regression for NDS 256x192 screens.

The final library footer used to start at y=178 with up to 16-pixel UI
glyphs; its descenders could extend below the 192-pixel LCD.
"""
from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[3]
BACKEND=ROOT/"cpymo-backends/nds/source"
LAUNCHER=(BACKEND/"launcher.h").read_text(encoding="utf8")
MAIN=(BACKEND/"main.c").read_text(encoding="utf8")
L10N=(BACKEND/"ui-language-nds.c").read_text(encoding="utf8")
EXTRA=(BACKEND/"ui-language-extra.h").read_text(encoding="utf8")

class LauncherHelpLayoutTests(unittest.TestCase):
 def test_two_library_footer_rows_have_bottom_padding_and_no_card_overlap(self):
  self.assertIn("int y=33+row*31",LAUNCHER)
  # No restyling: original 28px row rectangles and touch hit zone remain.
  self.assertIn("launcher_rect(6,y,244,28",LAUNCHER)
  self.assertIn("t.py>=33&&t.py<157",LAUNCHER)
  for y in (156,174):
   self.assertRegex(LAUNCHER,rf"launcher_text\([^;]+,7,{y},")
  self.assertGreaterEqual(156-(33+3*31+28),2)
  self.assertGreaterEqual(192-(174+16),2)
  self.assertEqual(174-156,18)
 def test_splash_uses_saved_language_before_first_render(self):
  main=MAIN[MAIN.index("int main(int argc,char** argv)"):]
  self.assertLess(main.index("fatInitDefault()"),main.index("nds_language_load()"))
  self.assertLess(main.index("nds_language_load()"),main.index("launcher_show_splash()"))
  self.assertIn('nds_ui("正在读取游戏列表...")',LAUNCHER)
 def test_all_enhanced_locales_explain_down_without_new_arrow_glyph(self):
  self.assertIn('nds_ui("A / DOWN / 轻触：继续")',MAIN)
  self.assertIn('{"A / DOWN / 轻触：继续","A / DOWN / Touch: Next"}',L10N)
  self.assertIn('"A / DOWN / 輕觸：繼續"',EXTRA)
  self.assertIn('"A / DOWN / タッチ: 進む"',EXTRA)
  self.assertNotIn('nds_ui("A / 轻触：继续")',MAIN)

if __name__=="__main__":
 unittest.main()
