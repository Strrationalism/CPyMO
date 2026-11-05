"""Test the exact NDS three-mode config parser/formatter under host GCC.
No ROM or game files required. Verifies all modes and legacy migration.
"""
import shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HEADER=ROOT/"cpymo-backends/nds/source"

C_TEST=r"""
#include "display-mode-prefs-nds.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define EXPECT(c) do{if(!(c))abort();}while(0)
int main(void){
 char line[32];
 for(unsigned mode=0;mode<3;mode++){
  EXPECT(nds_display_pref_format(line,sizeof line,mode));
  EXPECT(strchr(line,'\n')!=NULL);
  EXPECT(nds_display_pref_parse(line)==mode);
 }
 EXPECT(nds_display_pref_parse("1")==2);
 EXPECT(nds_display_pref_parse("1\n")==2);
 EXPECT(nds_display_pref_parse("1\r\n")==2);
 EXPECT(nds_display_pref_parse("NDSDISPLAY2 2\n")==2);
 EXPECT(nds_display_pref_parse("NDSDISPLAY2 1\r\n")==1);
 EXPECT(nds_display_pref_parse("NDSDISPLAY2 0")==0);
 EXPECT(nds_display_pref_parse("NDSDISPLAY2 3")==0);
 EXPECT(nds_display_pref_parse("NDSDISPLAY2 1 junk")==0);
 EXPECT(nds_display_pref_parse("NDSDISPLAY2 -1")==0);
 EXPECT(nds_display_pref_parse("broken")==0);
 EXPECT(nds_display_pref_parse(NULL)==0);
 EXPECT(!nds_display_pref_format(line,3,2));
 EXPECT(!nds_display_pref_format(line,sizeof line,3));
 /* Deliberately do a physical file roundtrip: CRLF normalization is not
    equivalent to just checking a parser with an absent newline. */
 FILE*f=tmpfile();EXPECT(f!=NULL);
 EXPECT(nds_display_pref_format(line,sizeof line,2));
 EXPECT(fputs(line,f)!=EOF);
 rewind(f);
 char readback[32]={0};
 EXPECT(fgets(readback,sizeof readback,f)!=NULL);
 EXPECT(nds_display_pref_parse(readback)==2);
 fclose(f);
 return 0;
}
"""
@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class DisplayModePrefTests(unittest.TestCase):
 def test_compile_and_roundtrip(self):
  with tempfile.TemporaryDirectory() as t:
   td=Path(t);src=td/"check.c";src.write_text(C_TEST,encoding="utf-8")
   exe=td/"check"
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-fsanitize=undefined","-I",str(HEADER),str(src),
                   "-o",str(exe)],check=True)
   subprocess.run([str(exe)],check=True,capture_output=True,text=True)
