"""FVDS 47,605-Hz stereo staging regression using PRODUCTION C sources.

No game media or FastVideoDS player code is embedded. Tests bounded queue
wraparound, overflow, silence on underrun, and 12-fps audio pacing from the
actual 3-frame synthetic FVDS packet source.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_fvds_inspect import sample

ROOT=Path(__file__).resolve().parents[3]
INC=ROOT/"cpymo-backends/nds/source"
C_FILES=["fvds-pcm-queue-nds.c","fvds-stream-nds.c",
         "fvds-adpcm-nds.c","fvds-stage-nds.c"]
DRIVER=r"""
#include "fvds-pcm-queue-nds.h"
#include "fvds-av-clock-nds.h"
#include "fvds-stage-nds.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define CHECK(x) do{if(!(x))abort();}while(0)

typedef struct {
 nds_fvds_pcm_queue queue;
 unsigned video_bytes;
 unsigned video_frames;
} test_context;
static int begin(void *ctx,uint32_t frame,uint64_t pts){
 test_context *c=ctx;
 CHECK(frame==c->video_frames);
 CHECK(pts==(uint64_t)frame*1000000/12);
 return 0;
}
static int chunk(void *ctx,const uint8_t *p,size_t n){
 test_context *c=ctx;
 CHECK(p && n && n<=512);
 c->video_bytes+=(unsigned)n;
 return 0;
}
static int end(void *ctx,uint32_t frame){
 test_context *c=ctx;
 CHECK(frame==c->video_frames++);
 return 0;
}
static int pcm(void *ctx,const int16_t *p,size_t frames){
 test_context *c=ctx;
 CHECK(frames==256);
 for(size_t i=0;i<frames*2;i++)CHECK(p[i]==0);
 return nds_fvds_pcm_queue_push(&c->queue,p,frames)?0:-1;
}
static void test_boundaries(void){
 int16_t memory[2*256]={0};
 int16_t in[2*256],out[2*300];
 for(unsigned i=0;i<512;i++)in[i]=(int16_t)(i-256);
 nds_fvds_pcm_queue q;
 CHECK(!nds_fvds_pcm_queue_init(&q,memory,0));
 CHECK(nds_fvds_pcm_queue_init(&q,memory,256));
 CHECK(nds_fvds_pcm_queue_push(&q,in,256));
 size_t previous=q.queued;
 CHECK(!nds_fvds_pcm_queue_push(&q,in,1));
 CHECK(q.overflows==1 && q.queued==previous);
 CHECK(nds_fvds_pcm_queue_drain(&q,out,200)==200);
 for(unsigned i=0;i<400;i++)CHECK(out[i]==in[i]);
 CHECK(nds_fvds_pcm_queue_push(&q,in,200));
 CHECK(q.queued==256);
 CHECK(nds_fvds_pcm_queue_drain(&q,out,300)==256);
 for(unsigned i=0;i<56*2;i++)CHECK(out[i]==in[200*2+i]);
 for(unsigned i=0;i<200*2;i++)CHECK(out[56*2+i]==in[i]);
 for(unsigned i=256*2;i<300*2;i++)CHECK(out[i]==0);
 CHECK(q.underflows==44 && q.consumed==456);
 nds_fvds_pcm_queue_reset(&q);
 CHECK(!q.queued && !q.underflows && !q.overflows);
}
static void test_two_picture_preroll(const char *filename){
 nds_fvds_stream stream;
 nds_fvds_stage stage={0};
 int16_t memory[2*8192],output[2*4096];
 test_context c={0};
 const nds_fvds_stage_callbacks callbacks={NULL,begin,chunk,end,pcm};
 CHECK(nds_fvds_pcm_queue_init(&c.queue,memory,8192));
 CHECK(nds_fvds_open(&stream,filename)==0);
 /* Decode two genuine packets before the presentation clock starts.
    Do not inject fake silence as an artificial warmup. */
 CHECK(nds_fvds_stage_step(&stream,&stage,&callbacks,&c)==0);
 CHECK(nds_fvds_stage_step(&stream,&stage,&callbacks,&c)==0);
 CHECK(c.queue.queued==8192);
 for(unsigned presentation=0;presentation<3;presentation++){
  size_t demand=nds_fvds_pcm_interval(presentation,12,1,47605);
  CHECK(demand<=4096);
  CHECK(nds_fvds_pcm_queue_drain(&c.queue,output,demand)==demand);
  for(size_t i=0;i<2*demand;i++)CHECK(output[i]==0);
  /* Once presentation consumes a picture interval, exactly one upcoming
     picture may be produced into the regained queue headroom. */
  if(presentation+2<3)
   CHECK(nds_fvds_stage_step(&stream,&stage,&callbacks,&c)==0);
 }
 CHECK(stage.completed_frames==3);
 CHECK(c.queue.underflows==0 && c.queue.overflows==0);
 CHECK(c.queue.high_watermark==8192);
 CHECK(nds_fvds_stage_step(&stream,&stage,&callbacks,&c)==1);
 nds_fvds_close(&stream);
}
int main(int argc,char **argv){
 CHECK(argc==2);
 test_boundaries();
 test_two_picture_preroll(argv[1]);
 int16_t memory[2*8192];
 int16_t out[2*4096];
 test_context c={0};
 CHECK(nds_fvds_pcm_queue_init(&c.queue,memory,8192));
 nds_fvds_stream file;
 CHECK(nds_fvds_open(&file,argv[1])==0);
 nds_fvds_stage stage={0};
 const nds_fvds_stage_callbacks callbacks={NULL,begin,chunk,end,pcm};
 size_t previously_drained=0;
 for(unsigned frame=0;frame<3;frame++){
  CHECK(nds_fvds_stage_step(&file,&stage,&callbacks,&c)==0);
  size_t target=nds_fvds_pcm_target(frame+1,12,1,47605);
  size_t delta=nds_fvds_pcm_interval(frame,12,1,47605);
  CHECK(delta==target-previously_drained);
  CHECK(delta<4096);
  CHECK(nds_fvds_pcm_queue_drain(&c.queue,out,delta)==delta);
  for(size_t j=0;j<delta*2;j++)CHECK(out[j]==0);
  previously_drained=target;
 }
 CHECK(nds_fvds_stage_step(&file,&stage,&callbacks,&c)==1);
 CHECK(c.video_frames==3 && c.video_bytes==24);
 CHECK(stage.completed_frames==3);
 CHECK(stage.audio_frames==47*256);
 CHECK(c.queue.queued==47*256-previously_drained);
 CHECK(c.queue.high_watermark>=4096);
 CHECK(c.queue.underflows==0 && c.queue.overflows==0);
 CHECK(c.queue.produced==47*256 && c.queue.consumed==previously_drained);
 CHECK(nds_fvds_pcm_target(1908,12,1,47605)==7569195ULL);
 CHECK(nds_fvds_pcm_interval(1907,12,1,47605)>=3967);
 CHECK(nds_fvds_pcm_interval(1907,12,1,47605)<=3968);
 CHECK(nds_fvds_pcm_interval(0,0,1,47605)==0);
 nds_fvds_close(&file);
 return 0;
}
"""

@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class FVDSPCMQueueTests(unittest.TestCase):
 def test_ring_and_paced_stage(self):
  with tempfile.TemporaryDirectory() as folder:
   d=Path(folder)
   runner=d/"runner.c";runner.write_text(DRIVER,encoding="utf-8")
   exe=d/"runner"
   subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                   "-fsanitize=undefined","-I",str(INC),
                   *(str(INC/c) for c in C_FILES),str(runner),
                   "-o",str(exe)],check=True,capture_output=True,text=True)
   fixture=d/"synthetic.fv";fixture.write_bytes(sample())
   subprocess.run([str(exe),str(fixture)],check=True,
                  capture_output=True,text=True)
