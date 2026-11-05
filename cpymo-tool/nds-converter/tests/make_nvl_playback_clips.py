"""Produce 1s NVL1 and 3s NVA1 *original* animated clips for NDS hardware.

The NVL1/NVA1 framing is from CPyMO's own NDS research player:
RGB555 256x192 frames, 12 FPS, LZ4 byte packets, Adler32 reconstructed CRC.
This test fixture does not embed media from commercial games.
"""
import hashlib,math,struct,sys,tempfile,zlib
from pathlib import Path
from PIL import Image, ImageDraw
import lz4.block

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import lossless


def generate(target, name, frames, compressed_audio):
    target=Path(target);target.mkdir(parents=True,exist_ok=True)
    assert name in ("NVL1S","NVA3S") and frames in (12,36)
    width,height,fps,rate=256,192,12,22050
    packet=bytearray()
    last=None
    for i in range(frames):
        im=Image.new("RGB",(width,height),(13,34+(i*3)%32,55))
        d=ImageDraw.Draw(im)
        for x in range(0,width,16):
            d.line((x,0,x,height),fill=(21,60,83))
        d.rectangle((15+i*5%184,50,60+i*5%184,97),fill=(235,191,89),outline=(255,239,164),width=2)
        d.ellipse((159,48+i*2%68,220,109+i*2%68),fill=(72,136,186))
        d.text((12,12),f"CPyMO 1.2  {name}",fill=(245,236,205))
        d.text((12,160),f"FRAME {i+1:02} / {frames:02}",fill=(233,211,170))
        pixels=im.tobytes()
        frame=bytearray(width*height*2)
        for p in range(width*height):
            r,g,b=pixels[p*3:p*3+3]
            struct.pack_into("<H",frame,p*2,0x8000|(r>>3)|((g>>3)<<5)|((b>>3)<<10))
        delta=bytes(frame) if last is None else bytes(a^b for a,b in zip(frame,last))
        raw=bytes(frame)
        encoded=lz4.block.compress(delta,store_size=False,mode="high_compression",compression=9)
        flags=1+(2 if last is not None else 0)
        packet+=struct.pack("<3I",len(encoded),flags,zlib.adler32(raw))+encoded
        last=raw
    samples=frames*rate//fps
    pcm=bytearray(samples*2)
    for i in range(samples):
        t=i/rate
        amp=2600+600*math.sin(2*math.pi*t)
        freq=330 if int(t*4)%2==0 else 440
        struct.pack_into("<h",pcm,i*2,int(amp*math.sin(2*math.pi*freq*t)))
    magic=b"NVA1" if compressed_audio else b"NVL1"
    with tempfile.TemporaryDirectory() as tmp:
        rawfile=Path(tmp)/"audio.pcm";rawfile.write_bytes(pcm)
        if compressed_audio:
            audfile=Path(tmp)/"audio.nar"
            audit=lossless.encode_audio(rawfile,audfile)
            audio=audfile.read_bytes()
            assert audio[:4]==b"NAR2" and audit["samples"]==samples
        else:
            audio=bytes(pcm)
    head=struct.pack("<4s7I",magic,width,height,fps,frames,rate,samples,32+len(packet))
    path=target/(name+".nvl")
    path.write_bytes(head+packet+audio)
    return {"name":name,"frames":frames,"audio_samples":samples,"size_bytes":path.stat().st_size,
            "sha256":hashlib.sha256(path.read_bytes()).hexdigest()}


if __name__=="__main__":
    out=Path(sys.argv[1]);print(generate(out,"NVL1S",12,False))
    print(generate(out,"NVA3S",36,True))
