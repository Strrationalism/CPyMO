"""Create a tiny original PyMO 1.2 FVDS *preflight* test game.

It deliberately contains synthetic *container-only* clips; they do not encode
visible movie frames or real music. Official converter generates the actual NDS
cache and verifies each packet; media decoder integration is a separate gate.
"""
import struct
import sys
from pathlib import Path
from PIL import Image, ImageDraw

ROOT=Path(sys.argv[1]).resolve()
for d in ("bg","bgm","script","video","save"):
    (ROOT/d).mkdir(parents=True,exist_ok=True)
(ROOT/"gameconfig.txt").write_text(
    "gametitle,FVDS视频资源检查\n"
    "platform,s60v3\nimagesize,320,240\nscripttype,pymo\n"
    "startscript,START\nengineversion,0.5\n"
    "bgformat,.png\ncharaformat,.png\ncharamaskformat,.png\n"
    "bgmformat,.wav\nvoiceformat,.wav\nseformat,.wav\n"
    "bgmvolume,5\nvovolume,5\nsevolume,5\nplayvideo,1\n"
    "fontsize,16\ntextcolor,#ffffff\n",encoding="utf-8")
script=[
    "#label LOOP",
    "#bg TEST,BG_NOFADE,0",
    "#bgm TESTBGM,1",
    "#say 系统,这是 CPyMO 1.2 综合视频测试。",
    "#say 系统,继续后检查第一段短视频容器。",
    "#movie CHECK1",
    "#say 系统,第一段返回剧情，请稍后查看检测报告。",
    "#say 系统,继续后检查第二段短视频容器。",
    "#movie CHECK3",
    "#say 系统,接下来读取完整 FVDS 视频与音频数据。",
    "#movie FULLJ1",
    "#say 系统,FVDS 全片读取结束。接下来检查已有 NVL1 播放器。",
    "#say 系统,短片一：一秒画面及声音，等待自然结束。",
    "#movie NVL1S",
    "#say 系统,一秒视频已经返回剧情。",
    "#say 系统,短片二：三秒画面及压缩音频，等待自然结束。",
    "#movie NVA3S",
    "#say 系统,三秒视频已经返回剧情。",
    "#say 系统,短片三：二十秒画面。可以按 B 中途跳过。",
    "#movie NVL20S",
    "#say 系统,长视频已经返回剧情。最后测试缺失视频。",
    "#movie NOFILE",
    "#say 系统,缺失视频处理结束。请检查 save 中的各项报告。",
    "#say 系统,如果画面和背景音乐恢复正常，说明测试流程完成。",
    "#goto LOOP",
]
(ROOT/"script"/"START.txt").write_text("\n".join(script)+"\n",encoding="utf-8")
# A genuine looping background theme tests stream restoration after NVL.
import wave, math
rate=22050
with wave.open(str(ROOT/"bgm"/"TESTBGM.wav"),"wb") as out:
    out.setnchannels(1);out.setsampwidth(2);out.setframerate(rate)
    wave_data=bytearray()
    for n in range(rate*4):
        t=n/rate
        freq=[220,262,330,294][int(t*2)%4]
        wave_data+=struct.pack("<h",int(2500*math.sin(2*math.pi*freq*t)))
    out.writeframes(wave_data)

im=Image.new("RGB",(320,240),(16,32,55))
dr=ImageDraw.Draw(im)
for y in range(240):
    dr.line((0,y,319,y),fill=(16+y//15,32+y//10,55+y//7))
for x in range(12,309,17):
    dr.line((x,26,x,212),fill=(31,66,93),width=1)
for y in range(26,213,17):
    dr.line((12,y,307,y),fill=(31,66,93),width=1)
dr.rectangle((28,55,291,177),outline=(144,192,200),width=3)
dr.rectangle((42,82,278,154),fill=(32,51,77))
im.save(ROOT/"bg"/"TEST.png")
# The opt-in flag is placed in converted save/ AFTER conversion.
def synthetic_fvds(frames):
    data=bytearray(struct.pack("<4sHHIIHHII",b"FVDS",256,192,12,1,47605,2,frames,1))
    data+=struct.pack("<II",0,36)
    for i in range(frames):
        blocks=16 if i%2 else 15
        video=bytes([i&255])*32
        data+=struct.pack("<I",(blocks<<17)|len(video))
        data+=video+bytes(blocks*264)
    return data
(ROOT/"video"/"CHECK1.fv").write_bytes(synthetic_fvds(12))
(ROOT/"video"/"CHECK3.fv").write_bytes(synthetic_fvds(36))
(ROOT/"video"/"FULLJ1.fv").write_bytes(synthetic_fvds(1908))
print("Created synthetic FVDS preflight source:",ROOT)
