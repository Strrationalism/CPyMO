"""Test only FVDS I/P header semantics, not video image decoding.
Synthetic words model the documented encoder quantizer bit placement.
"""
import shutil,subprocess,tempfile,unittest
from pathlib import Path

SRC=Path(__file__).resolve().parents[3]/"cpymo-backends/nds/source"
RUNNER=r"""
#include "fvds-picture-prefix-nds.h"
#include <assert.h>
int main(void){
 const uint8_t iframe[2]={0x0f,0x3c}; /* 0 + quantizer 30 */
 const uint8_t pframe[2]={0xff,0xff}; /* P flag */
 assert(nds_fvds_picture_type(iframe,2,1)==0);
 assert(nds_fvds_intra_quantizer(iframe,2)==30);
 assert(nds_fvds_picture_type(pframe,2,0)==1);
 assert(nds_fvds_picture_type(pframe,2,1)==-2);
 assert(nds_fvds_intra_quantizer(pframe,2)==-1);
 assert(nds_fvds_picture_type(iframe,1,0)==-1);
 assert(nds_fvds_picture_type(NULL,0,0)==-1);
 /* 6-bit quantizer should not shift with signedness. */
 const uint8_t maxq[2]={0x00,0x7e};
 assert(nds_fvds_intra_quantizer(maxq,2)==63);
 return 0;
}
"""
@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class FVDSPicturePrefixTests(unittest.TestCase):
 def test_intra_and_predicted_header(self):
  with tempfile.TemporaryDirectory() as tmp:
   folder=Path(tmp)
   c=folder/"probe.c";c.write_text(RUNNER,encoding="utf-8")
   exe=folder/"probe"
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-fsanitize=undefined","-I",str(SRC),str(c),
                   "-o",str(exe)],check=True,capture_output=True,text=True)
   subprocess.run([str(exe)],check=True,capture_output=True,text=True)
