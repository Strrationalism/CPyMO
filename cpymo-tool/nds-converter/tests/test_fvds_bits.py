"""Exercise the PRODUCTION C elementary FVDS picture bit reader.

No external decoder source is compiled or copied.  The test makes synthetic
little-endian, MSB-first words and verifies exact 512-byte block crossings,
I-picture q30, stream bounds and short-card-read rejection.
"""
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
SRC=ROOT/"cpymo-backends/nds/source"

C_TEST=r"""
#include "fvds-bits-nds.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(e) do{if(!(e))abort();}while(0)
typedef struct {
 const uint8_t *data;
 unsigned length,pos,calls;
 int fail_early;
} memory_source;
static size_t pull(void *ctx,void *dst,size_t request){
 memory_source*s=ctx;
 s->calls++;
 if(s->fail_early && s->calls==1)return 0;
 if(request>s->length-s->pos)request=s->length-s->pos;
 memcpy(dst,s->data+s->pos,request);
 s->pos+=(unsigned)request;
 return request;
}
static void split_and_bounds(void){
 uint8_t video[514]={0};
 /* Four source byte pairs prove the storage endian and bit direction. */
 video[0]=0x0f;video[1]=0x3c; /* 0 + six-bit intra Q=30 */
 video[2]=0x00;video[3]=0x80; /* 1 + remaining bits 0 */
 video[510]=0xff;video[511]=0x7f; /* last word of first chunk */
 video[512]=0xca;video[513]=0xad; /* first word of next chunk */
 memory_source src={video,sizeof video,0,0,0};
 nds_fvds_bits bit;
 CHECK(nds_fvds_bits_init(&bit,pull,&src,sizeof video));
 uint32_t value=~0u;
 CHECK(nds_fvds_bits_read(&bit,1,&value)&&value==0);
 CHECK(nds_fvds_bits_read(&bit,6,&value)&&value==30);
 CHECK(nds_fvds_bits_read(&bit,9,&value)&&value==0x00f);
 CHECK(nds_fvds_bits_read(&bit,1,&value)&&value==1);
 CHECK(nds_fvds_bits_align16(&bit));
 CHECK(bit.consumed_bits==32);
 for(unsigned i=2;i<255;i++)
  CHECK(nds_fvds_bits_read(&bit,16,&value)&&value==0);
 CHECK(nds_fvds_bits_read(&bit,16,&value)&&value==0x7fff);
 CHECK(nds_fvds_bits_read(&bit,16,&value)&&value==0xadca);
 CHECK(!nds_fvds_bits_read(&bit,1,&value)&&bit.failed);
 CHECK(src.pos==514 && src.calls==2);
 CHECK(!nds_fvds_bits_read(&bit,1,&value));
}
static void rejects_bad_data(void){
 uint8_t data[8]={0};
 memory_source src={data,8,0,0,1};
 nds_fvds_bits b;
 CHECK(!nds_fvds_bits_init(&b,pull,&src,0));
 CHECK(!nds_fvds_bits_init(&b,pull,&src,3));
 CHECK(!nds_fvds_bits_init(&b,pull,&src,131072));
 CHECK(nds_fvds_bits_init(&b,pull,&src,8));
 uint32_t v=0x12345678;
 CHECK(!nds_fvds_bits_read(&b,17,&v)&&v==0x12345678);
 CHECK(!nds_fvds_bits_read(&b,1,&v)&&b.failed);
 CHECK(v==0x12345678);
 /* Short reader cannot cause out-of-bounds source consumption. */
 src.fail_early=0;
 src.calls=0;
 CHECK(nds_fvds_bits_init(&b,pull,&src,16));
 CHECK(!nds_fvds_bits_read(&b,1,&v));
 CHECK(b.failed && src.pos==8);
}
int main(void){split_and_bounds();rejects_bad_data();return 0;}
"""

@unittest.skipUnless(shutil.which("gcc"),"gcc not installed")
class FVDSBitsTests(unittest.TestCase):
 def test_endian_word_boundary_and_failed_pull(self):
  with tempfile.TemporaryDirectory() as td:
   d=Path(td)
   c=d/"check.c";c.write_text(C_TEST,encoding="utf-8")
   runner=d/"runner"
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-fsanitize=undefined","-I",str(SRC),
                   str(SRC/"fvds-bits-nds.c"),str(c),"-o",str(runner)],
                  check=True,capture_output=True,text=True)
   subprocess.run([str(runner)],check=True,capture_output=True,text=True)
