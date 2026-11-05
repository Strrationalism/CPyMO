"""v1.2.1: host-level guards for themes, title help and rendering."""
from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[3]
SRC=ROOT/"cpymo-backends/nds/source"
def load(name):return (SRC/name).read_text(encoding="utf8")

class NdsV121Tests(unittest.TestCase):
 def test_ipl_favorite_colors_exact(self):
  theme=load("theme-nds.h")
  match=re.search(r'nds_ipl_theme_rgb\[16\]\[3\]=\{(.*?)\};',theme,re.S)
  self.assertIsNotNone(match)
  rgb=[tuple(map(int,x)) for x in re.findall(r'\{(\d+),(\d+),(\d+)\}',match.group(1))]
  self.assertEqual(rgb,[
   (12,16,19),(23,9,0),(31,0,3),(31,17,31),
   (31,18,0),(30,28,0),(21,31,0),(0,31,0),
   (0,20,7),(9,27,17),(6,23,30),(0,11,30),
   (0,0,18),(17,0,26),(26,0,29),(31,0,18)])
  self.assertIn('PersonalData->theme',theme)
  self.assertIn('/pymo/nds-theme.cfg',theme)
 def test_launcher_shortcuts(self):
  launcher=load("launcher.h")
  for text in ('if(down&(KEY_A|KEY_START))start=true;',
               'if(down&KEY_Y){launcher_theme_menu',
               'nds_ui("A / START 启动 · 轻触两次启动")',
               'nds_ui("上下选择 · B 关于 · X 语言 · Y 主题")',
               'launcher_rect(x,y,57,21,nds_theme_color(i))'):
   self.assertIn(text,launcher)
 def test_title_help_and_alpha_rounding(self):
  launcher=load("launcher.h")
  main=load("main.c")
  self.assertIn('nds_dual_title_allow_split(&nds_title_gate)',launcher)
  self.assertIn('nds_ui("UP / DOWN：选择项目")',launcher)
  self.assertIn('nds_ui("L：切换显示模式")',launcher)
  self.assertIn('else if(!debug_trace)launcher_play_panel(NULL)',main)
  self.assertIn('0x4010u',main)
  self.assertIn('0x0200u',main)

 def test_nondefault_colors_are_true_theme_families(self):
  theme=load("theme-nds.h");launcher=load("launcher.h")
  self.assertIn('nds_theme_tint(unsigned idx,unsigned strength,unsigned neutral)',theme)
  self.assertIn('RGB15(3,5,8),RGB15(5,10,16),RGB15(4,7,11),RGB15(7,15,23)',theme)
  self.assertIn('nds_theme_tint(i,6,1),nds_theme_tint(i,11,3)',theme)
  self.assertIn('nds_theme_tint(i,9,3),nds_theme_tint(i,11,4)',theme)
  self.assertIn('RGB15(15,26,30)};',theme) # approved Default B rail
  self.assertIn('nds_theme_tint(i,13,17),RGB15(30,30,30)',theme)
  self.assertIn('nds_theme_tint(i,3,23),nds_theme_tint(i,3,24)',theme)
  self.assertIn('nds_theme_tint(i,2,24),nds_theme_tint(i,3,23)',theme)
  self.assertIn('u16 white=nds_theme_primary(),muted=nds_theme_muted(),accent=nds_theme_accent();',launcher)
  self.assertNotIn('launcher_theme_header_trim',launcher)
  self.assertNotIn('launcher_rect(6,y,3,28',launcher)
  self.assertIn('if(index==launcher_selected)launcher_rect(6,y,2,28,nds_theme_rail());',launcher)
  mat=re.search(r'nds_ipl_theme_rgb\[16\]\[3\]=\{(.*?)\};',theme,re.S)
  colors=[tuple(map(int,m)) for m in re.findall(r'\{(\d+),(\d+),(\d+)\}',mat.group(1))]
  self.assertEqual(len(colors),16)
  def tinted(c,s,n):return tuple(n+(v*s+15)//31 for v in c)
  def luminance(rgb):
   def linear(c):
    x=c/31
    return x/12.92 if x<=.04045 else ((x+.055)/1.055)**2.4
   return sum(weight*linear(x) for weight,x in zip((.2126,.7152,.0722),rgb))
  for c in colors:
   primary=(30,30,30);selected=tinted(c,11,4)
   self.assertGreaterEqual((luminance(primary)+.05)/(luminance(selected)+.05),4.5)
   for strength,neutral in ((6,1),(11,3),(9,3),(11,4),(13,17),
                            (3,23),(3,24),(2,24),(13,13)):
    self.assertTrue(all(0<=v<=31 for v in tinted(c,strength,neutral)))
  red=tinted(colors[2],6,1)
  self.assertGreater(red[0],red[2])
 def test_auto_palette_is_cached_across_all_redraws(self):
  theme=load("theme-nds.h");launcher=load("launcher.h")
  self.assertIn('static int nds_theme_effective_index=-1;',theme)
  self.assertIn('static int nds_theme_active_index(void){return nds_theme_effective_index;}',theme)
  self.assertEqual(theme.count('nds_theme_firmware()'),1)
  self.assertIn('if(ok){nds_theme_preference=v;nds_theme_resolve();}',theme)
  self.assertIn('const u16 color=nds_theme_bg()|BIT(15)',launcher)
  self.assertIn('static u16 nds_theme_bg(void){return nds_theme_scheme.bg;}',theme)
 def test_auto_firmware_snapshot_on_host(self):
  import shutil,subprocess,tempfile
  if not shutil.which("gcc"):self.skipTest("gcc unavailable")
  program=r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
typedef uint16_t u16;
#define RGB15(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
struct FakeEnv { unsigned version; };
struct FakePersonal { unsigned settingsLost,theme; };
static struct FakeEnv env={5};
static struct FakePersonal pd={0,2};
static struct FakeEnv* g_envUserSettings=&env;
static struct FakePersonal* PersonalData=&pd;
static const char* nds_ui(const char* text){return text;}
static int nds_language_effective(void){return 0;}
#include "theme-nds.h"
int main(void){
 nds_theme_preference=0;nds_theme_resolve();
 assert(nds_theme_bg()==RGB15(3,5,8));
 assert(nds_theme_primary()==RGB15(31,31,31));
 assert(nds_theme_rail()==RGB15(15,26,30));
 nds_theme_preference=1;nds_theme_resolve();
 assert(nds_theme_active_index()==2);
 const u16 first=nds_theme_bg();
 pd.theme=7;
 for(int i=0;i<100000;i++)assert(nds_theme_bg()==first);
 assert(nds_theme_active_index()==2);
 nds_theme_resolve();
 assert(nds_theme_active_index()==7);
 assert(nds_theme_bg()!=first);
 for(unsigned i=0;i<16;i++){
  nds_theme_preference=i+2;nds_theme_resolve();
  assert(nds_theme_active_index()==(int)i);
  assert(nds_theme_primary()==RGB15(30,30,30));
  assert(nds_theme_accent()==nds_theme_tint(i,13,17));
  assert(nds_theme_muted()==nds_theme_tint(i,3,23));
  assert(nds_theme_detail()==nds_theme_tint(i,3,24));
  assert(nds_theme_clock_ink()==nds_theme_tint(i,2,24));
  assert(nds_theme_aux_clock_ink()==nds_theme_tint(i,3,23));
  assert(nds_theme_rail()==nds_theme_tint(i,13,13));
 }
 pd.settingsLost=1;nds_theme_preference=1;nds_theme_resolve();
 assert(nds_theme_active_index()==-1);
 return 0;
}
"""
  with tempfile.TemporaryDirectory() as td:
   from pathlib import Path
   p=Path(td);(p/"calico/nds").mkdir(parents=True)
   (p/"calico/nds/env.h").write_text("/* host shim */\n",encoding="utf8")
   (p/"main.c").write_text(program,encoding="utf8")
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-Wno-unused-function","-fsanitize=undefined",
                   "-I",str(p),"-I",str(SRC),
                   str(p/"main.c"),"-o",str(p/"program")],
                  check=True,capture_output=True,text=True)
   subprocess.run([str(p/"program")],check=True,capture_output=True,text=True)
 def test_default_and_auto_labels_are_centered(self):
  launcher=load("launcher.h")
  self.assertIn('launcher_text_pixel_width(',launcher)
  self.assertIn('launcher_text_centered(nds_theme_choice_name(i),x,w,36,white)',launcher)
  self.assertNotIn('nds_theme_choice_name(i),x+30',launcher)
 def test_restart_callback_rearms_title_guide(self):
  main=load("main.c");rmenu=load("cpymo_rmenu.c")
  self.assertIn('if (!err) nds_request_title_guide_reset();',rmenu)
  self.assertIn('if(!err&&nds_title_restart_pending)',main)
  self.assertIn('nds_dual_title_reset(&nds_title_gate);',main)
  self.assertIn('nds_aux_refresh();',main)

if __name__=="__main__":
 unittest.main()
