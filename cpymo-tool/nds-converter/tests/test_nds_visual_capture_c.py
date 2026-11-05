"""Run the actual NDS C framebuffer writer under host GCC (fake libnds types)
and verify its two files against the same native PNG decoder.

Synthetic pixels ONLY exercise the exporter; never represent visual screenshots
of CPyMO or emulator gameplay.
"""
from pathlib import Path
import importlib.util
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[3]
SOURCE=ROOT/"cpymo-backends/nds/source"
CONVERTER=ROOT/"cpymo-backends/nds/tools/nds-capture-to-png.py"
SPEC=importlib.util.spec_from_file_location("nds_capture_to_png",CONVERTER)
capture=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(capture)

C_TEST=r"""
#include <stdint.h>
#include <stddef.h>
typedef uint16_t u16;
static void nds_audio_pump_if_due(void){}
#include "visual-capture-nds.h"
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x))abort(); } while(0)
static u16 top[256*192],bottom[256*192];
int main(int argc,char**argv){
 CHECK(argc==2);
 nds_capture_init(argv[1]);
 CHECK(!nds_capture_armed);
 CHECK(nds_capture_begin(1,top)==NULL);
 char flag[512];
 int n=snprintf(flag,sizeof flag,"%s/save/nds-visual-capture.flag",argv[1]);
 CHECK(n>0 && (size_t)n<sizeof flag);
 FILE*f=fopen(flag,"wb");CHECK(f);CHECK(fclose(f)==0);
 nds_capture_init(argv[1]);
 CHECK(nds_capture_armed);
 for(unsigned i=0;i<256*192;i++){
  top[i]=(i&1)?0x83e0:0x801f;
  bottom[i]=(i&1)?0x8000:0xfc00;
 }
 FILE*shot=nds_capture_begin(1,top);
 CHECK(shot);
 nds_capture_finish(shot,1,bottom);
 CHECK(nds_capture_taken==1 && nds_capture_error==0);
 CHECK(!nds_capture_begin(1,top));
 shot=nds_capture_begin(2,top);
 CHECK(shot);
 nds_capture_finish(shot,2,bottom);
 CHECK(nds_capture_taken==3 && nds_capture_error==0);
 shot=nds_capture_begin(3,top);
 CHECK(shot);
 nds_capture_finish(shot,3,bottom);
 CHECK(nds_capture_taken==7 && nds_capture_error==0);
 shot=nds_capture_begin(4,top);
 CHECK(shot);
 nds_capture_finish(shot,4,bottom);
 CHECK(nds_capture_taken==15 && nds_capture_error==0);
 CHECK(!nds_capture_begin(4,top));
 CHECK(!nds_capture_begin(0,top));
 CHECK(!nds_capture_begin(5,top));
 return 0;
}
"""
@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class RealFramebufferWriterTests(unittest.TestCase):
 def test_opt_in_native_writer_and_png_converter(self):
  with tempfile.TemporaryDirectory() as temp:
   base=Path(temp);game=base/"game"
   (game/"save").mkdir(parents=True)
   driver=base/"driver.c"
   driver.write_text(C_TEST,encoding="utf-8")
   binary=base/"driver"
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-fsanitize=undefined","-I",str(SOURCE),str(driver),
                   "-o",str(binary)],check=True,capture_output=True,text=True)
   subprocess.run([str(binary),str(game)],check=True,
                  capture_output=True,text=True)
   for kind,name in ((1,"dialogue"),(2,"menu"),
                     (3,"choice"),(4,"classic-lower")):
    path=game/"save"/f"nds-capture-{name}.ndsraw"
    self.assertEqual(path.stat().st_size,capture.EXPECTED_BYTES)
    actual_kind,(top,bottom)=capture.decode_capture(path.read_bytes())
    self.assertEqual(actual_kind,kind)
    self.assertEqual(top[:6],bytes((255,0,0,0,255,0)))
    self.assertEqual(bottom[:6],bytes((0,0,255,0,0,0)))
    outputs=capture.convert(path)
    self.assertEqual(len(outputs),2)
    self.assertTrue(all(p.stat().st_size>200 for p in outputs))
