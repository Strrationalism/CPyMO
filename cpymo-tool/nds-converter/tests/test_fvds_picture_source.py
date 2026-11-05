"""Production FVDS packet -> 16-bit MSB-first bit reader -> ADPCM boundary.
Synthetic video crosses the 512-byte frame buffer.  No user media committed.
"""
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path
from test_fvds_inspect import sample

ROOT=Path(__file__).resolve().parents[3]
SRC=ROOT/"cpymo-backends/nds/source"
C_TEST=r"""
#include "fvds-picture-source-nds.h"
#include <stdlib.h>
#include <stdint.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FVDS stream test line %d failed: %s\\n",__LINE__,#x);abort();}}while(0)
int main(int argc,char **argv){
 CHECK(argc==2);
 nds_fvds_stream movie;
 nds_fvds_packet packet;
 nds_fvds_bits bits;
 CHECK(nds_fvds_open(&movie,argv[1])==0);
 CHECK(nds_fvds_next(&movie,&packet)==0);
 CHECK(packet.video_bytes==516u);
 CHECK(nds_fvds_picture_begin(&movie,&bits));
 uint32_t b=0;
 CHECK(nds_fvds_bits_read(&bits,1,&b) && b==0);
 CHECK(nds_fvds_bits_read(&bits,6,&b) && b==30);
 CHECK(nds_fvds_bits_read(&bits,9,&b) && b==15);
 /* Consume the second storage word's leading 1 before aligning over its
    remaining 15 padding bits. The previous test aligned at the boundary
    *before* this word, erroneously expecting a zero next. */
 CHECK(nds_fvds_bits_read(&bits,1,&b) && b==1);
 CHECK(nds_fvds_bits_align16(&bits));
 for(unsigned i=2;i<255;i++){
  int ok=nds_fvds_bits_read(&bits,16,&b);
  if(!ok || b!=0){
   fprintf(stderr,"word=%u value=%08x ok=%d fetched=%u remaining=%u used=%u\\n",
     i,(unsigned)b,ok,bits.fetched_bytes,bits.remaining_bits,bits.consumed_bits);
   abort();
  }
 }
 CHECK(nds_fvds_bits_read(&bits,16,&b)&&b==0x7fffu);
 CHECK(nds_fvds_bits_read(&bits,16,&b)&&b==0xadcau);
 /* Its remaining final 2 bytes are padding and must NOT spill into audio. */
 CHECK(nds_fvds_picture_to_audio(&movie,&bits));
 CHECK(movie.video_remaining==0);
 uint8_t compressed[264];
 for(unsigned i=0;i<packet.audio_blocks;i++)
  CHECK(nds_fvds_read_audio(&movie,compressed,264)==264);
 CHECK(movie.audio_remaining==0);
 CHECK(nds_fvds_finish(&movie)==0);
 CHECK(nds_fvds_next(&movie,&packet)==0);
 CHECK(packet.frame==1);
 CHECK(nds_fvds_finish(&movie)==0);
 CHECK(nds_fvds_next(&movie,&packet)==0);
 CHECK(nds_fvds_finish(&movie)==0);
 CHECK(nds_fvds_next(&movie,&packet)==1);
 nds_fvds_close(&movie);
 return 0;
}
"""
@unittest.skipUnless(shutil.which("gcc"),"gcc unavailable")
class FVDSReaderAdapterTests(unittest.TestCase):
 def test_packet_reads_never_enter_audio(self):
  old=sample()
  offset=36
  packed=struct.unpack_from("<I",old,offset)[0]
  blocks=packed>>17
  self.assertEqual(packed&0x1ffff,8)
  video=bytearray(516)
  video[:4]=b"\x0f\x3c\x00\x80"
  video[510:514]=b"\xff\x7f\xca\xad"
  extended=(old[:offset]+struct.pack("<I",(blocks<<17)|len(video))+
            bytes(video)+old[offset+4+8:])
  with tempfile.TemporaryDirectory() as td:
   d=Path(td)
   fixture=d/"fixture.fv";fixture.write_bytes(extended)
   code=d/"test.c";code.write_text(C_TEST,encoding="utf-8")
   exe=d/"test"
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-fsanitize=undefined","-I",str(SRC),
                   str(SRC/"fvds-stream-nds.c"),
                   str(SRC/"fvds-bits-nds.c"),
                   str(code),"-o",str(exe)],
                  check=True,capture_output=True,text=True)
   result=subprocess.run([str(exe),str(fixture)],
                          capture_output=True,text=True)
   self.assertEqual(result.returncode,0,result.stderr)
