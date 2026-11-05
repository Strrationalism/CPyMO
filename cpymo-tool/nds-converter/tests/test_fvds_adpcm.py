"""Host regression for independent FVDS ADPCM decoder (no game media).
Compiles the *actual NDS C file* and checks stereo sample order, nibble order,
clipping, index transitions and malformed data using synthetic inputs.
"""
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
SOURCE=ROOT/"cpymo-backends/nds/source/fvds-adpcm-nds.c"
INCLUDE=SOURCE.parent
STEPS=(7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,
       50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,
       230,253,279,307,337,371,408,449,494,544,598,658,724,796,
       876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,
       2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,
       8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,
       22385,24623,27086,29794,32767)
JUMPS=(-3,-3,-2,-1,2,4,6,8)

RUNNER=r"""
#include "fvds-adpcm-nds.h"
#include <stdio.h>
#include <stdint.h>
int main(int argc,char**argv){
 if(argc!=2)return 2;
 FILE*f=fopen(argv[1],"rb");if(!f)return 3;
 uint8_t in[264];int16_t out[512]={0};
 if(fread(in,1,sizeof in,f)!=sizeof in||fgetc(f)!=EOF)return 4;
 fclose(f);
 int n=nds_fvds_adpcm_pair(in,sizeof in,out,512);
 if(n<0)return 5;
 if(n!=256||fwrite(out,1,sizeof out,stdout)!=sizeof out)return 6;
 return 0;
}
"""

def channel(predictor,index,nibbles):
    samples=[]
    for nib in nibbles:
        diff=(STEPS[index]*(2*(nib&7)+1))>>3
        predictor+=-diff if (nib&8) else diff
        predictor=max(-32767,min(32767,predictor))
        samples.append(predictor)
        index=max(0,min(88,index+JUMPS[nib&7]))
    return samples

def make_pair(left_predictor,left_index,left_data,
              right_predictor,right_index,right_data):
    return (struct.pack("<hH",left_predictor,left_index)+left_data+
            struct.pack("<hH",right_predictor,right_index)+right_data)

def baseline(data):
    l,r=data[:132],data[132:]
    result=[]
    for b in (l,r):
        predictor,index=struct.unpack_from("<hH",b)
        nibbles=[nib for value in b[4:] for nib in (value&15,value>>4)]
        result.append(channel(predictor,index,nibbles))
    return struct.pack("<512h",*(x for pair in zip(*result) for x in pair))

@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class FVDSADPCMTests(unittest.TestCase):
    def test_block_pcm_and_reject_corruption(self):
        with tempfile.TemporaryDirectory() as d:
            d=Path(d);(d/"runner.c").write_text(RUNNER)
            exe=d/"runner"
            subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                            "-fsanitize=undefined","-I",str(INCLUDE),
                            str(SOURCE),str(d/"runner.c"),"-o",str(exe)],check=True)
            vectors=[
                make_pair(0,0,bytes(128),0,0,bytes(128)),
                make_pair(-1,3,bytes(range(128)),12000,67,bytes(255-i for i in range(128))),
                make_pair(-32000,88,bytes([0xff]*128),32000,88,bytes([0x77]*128)),
                make_pair(13579,23,bytes([0x18]*128),-13579,70,bytes([0xe2]*128)),
            ]
            for i,blob in enumerate(vectors):
                file=d/(str(i)+".fv-audio-pair");file.write_bytes(blob)
                p=subprocess.run([str(exe),str(file)],capture_output=True,check=True)
                self.assertEqual(p.stdout,baseline(blob))
                self.assertEqual(len(p.stdout),1024)
            for bad in (make_pair(0,89,bytes(128),0,0,bytes(128)),
                        make_pair(0,0,bytes(128),0,65535,bytes(128))):
                file=d/"bad";file.write_bytes(bad)
                p=subprocess.run([str(exe),str(file)],capture_output=True)
                self.assertEqual(p.returncode,5)

if __name__=="__main__":
    unittest.main()
