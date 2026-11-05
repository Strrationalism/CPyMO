"""Exercise the NDS save directory helper with real host filesystem writes.
This verifies the missing-save-folder regression without an emulator.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[3]
SOURCE=ROOT/"cpymo-backends/nds/source"
PROGRAM=r"""
#include "save-dir-nds.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define CHECK(expr) do{if(!(expr))abort();}while(0)
int main(int argc,char**argv){
 if(argc!=2)return 2;
 const char* game=argv[1];
 char savepath[768];
 snprintf(savepath,sizeof savepath,"%s/save",game);
 struct stat st;
 CHECK(stat(savepath,&st)!=0);
 CHECK(nds_save_fopen(game,"save-01.csav","rb")==NULL);
 FILE *f=nds_save_fopen(game,"save-01.csav","wb");
 CHECK(f!=NULL);
 CHECK(fwrite("good",1,4,f)==4);
 CHECK(fclose(f)==0);
 CHECK(stat(savepath,&st)==0 && S_ISDIR(st.st_mode));
 f=nds_save_fopen(game,"save-01.csav","rb");
 CHECK(f!=NULL);
 char bytes[5]={0};
 CHECK(fread(bytes,1,4,f)==4 && !strcmp(bytes,"good"));
 CHECK(fclose(f)==0);
 /* Re-creating an existing directory must not clobber its saves. */
 CHECK(nds_save_ensure_directory(game));
 f=nds_save_fopen(game,"save-01.csav","rb");
 CHECK(f!=NULL);
 CHECK(fread(bytes,1,4,f)==4 && !strcmp(bytes,"good"));
 CHECK(fclose(f)==0);
 CHECK(nds_save_fopen(game,"nds-ui.cfg","wb")!=NULL);
 return 0;
}
"""
@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class NDSSaveFolderTests(unittest.TestCase):
 def test_auto_mkdir_and_existing_slot_preserved(self):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp)
   folder=root/"fixture game";folder.mkdir()
   code=root/"test.c";code.write_text(PROGRAM,encoding="utf-8")
   exe=root/"test"
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-I",str(SOURCE),str(code),"-o",str(exe)],
                  capture_output=True,text=True,check=True)
   subprocess.run([str(exe),str(folder)],capture_output=True,text=True,check=True)
   self.assertEqual((folder/"save/save-01.csav").read_bytes(),b"good")

