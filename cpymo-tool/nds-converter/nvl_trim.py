#!/usr/bin/env python3
"""Extract beginning-only NVL1 test clips without transcoding.

The script copies original compressed frame packets and the corresponding
PCM16 audio prefix. It does not include any third-party codec or media and
never replaces the source. Trimming at frame zero preserves delta frames'
reference dependency; arbitrary start timestamps are deliberately unsupported.
"""
from pathlib import Path
import argparse
import struct

HEADER = struct.Struct("<4s7I")
PACKET = struct.Struct("<3I")


class NVLFormatError(ValueError):
    pass


def trim_first_frames(source, output, first_frames):
    source, output = Path(source), Path(output)
    if source.resolve() == output.resolve() or output.exists():
        raise ValueError("Output must be a new path distinct from input")
    with source.open("rb") as f:
        if source.stat().st_size < HEADER.size:
            raise NVLFormatError("Missing header")
        magic, width, height, fps, frames, sample_rate, samples, audio_start = HEADER.unpack(f.read(32))
        if (magic != b"NVL1" or (width,height)!=(256,192)
                or not (1<=fps<=30) or not (1<=frames<=fps*600)
                or sample_rate != 22050 or samples != frames*22050//fps
                or audio_start < 32+frames*PACKET.size):
            raise NVLFormatError("Unsupported NVL1 header")
        if source.stat().st_size != audio_start + samples*2:
            raise NVLFormatError("Invalid PCM audio boundary")
        if not (1<=first_frames<=frames):
            raise ValueError("Requested frame count outside source range")
        end = 32
        for index in range(frames):
            f.seek(end)
            payload = f.read(PACKET.size)
            if len(payload)!=12:
                raise NVLFormatError("Truncated video packet")
            n,flags,checksum = PACKET.unpack(payload)
            if not (1<=n<=98304) or flags&~3 or (index==0 and flags&2):
                raise NVLFormatError("Invalid video packet")
            end += PACKET.size+n
            if end > audio_start:
                raise NVLFormatError("Video spills into audio")
            if index+1==first_frames:
                stop=end
        if end!=audio_start:
            raise NVLFormatError("Trailing video bytes")
        target_samples=first_frames*sample_rate//fps
        target_offset=stop
        try:
            with output.open("xb") as dest:
                dest.write(HEADER.pack(magic,width,height,fps,first_frames,sample_rate,target_samples,target_offset))
                f.seek(32)
                remaining=stop-32
                while remaining:
                    block=f.read(min(8192,remaining))
                    if not block:raise NVLFormatError("Truncated source frames")
                    dest.write(block)
                    remaining-=len(block)
                f.seek(audio_start)
                remaining=target_samples*2
                while remaining:
                    block=f.read(min(8192,remaining))
                    if not block:raise NVLFormatError("Truncated source audio")
                    dest.write(block)
                    remaining-=len(block)
        except BaseException:
            output.unlink(missing_ok=True)
            raise
    return {"frames":first_frames,"fps":fps,
            "duration_seconds":first_frames/fps,
            "bytes":output.stat().st_size,
            "audio_samples":target_samples}


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source",type=Path)
    parser.add_argument("output",type=Path)
    parser.add_argument("frames",type=int)
    args=parser.parse_args()
    print(trim_first_frames(args.source,args.output,args.frames))
