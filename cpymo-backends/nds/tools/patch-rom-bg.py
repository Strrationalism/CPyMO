#!/usr/bin/env python3
"""Embed the approved 256x192 RGB555 artwork into an NDS ARM9 ROM image.

The ROM compiled from nds-picture-bg.h contains one immutable 98320-byte
placeholder. Replacement is in-place (identical file length, header/CRC/ARM7
untouched), so players need ONLY the .nds, never a separate /pymo file.

Usage:
  python patch-rom-bg.py --rom unpatched.nds --art nds-dual-bg.rgb555 \\
      --output CPyMO_1.2_DSpico_TWL.nds
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct

WAIT=b"CPYMO_BG_AWAIT12"
READY=b"CPYMO_BG_READY12"
PLACEHOLDER=bytes([0x5a,0x5a]) * (256*192)
ART_MAGIC=b"CPYBG12"+bytes([0])
ART_SIZE=8+len(PLACEHOLDER)

def embed(rom: Path, art: Path, dest: Path) -> dict:
    raw=rom.read_bytes()
    src=art.read_bytes()
    if len(src)!=ART_SIZE or src[:8]!=ART_MAGIC:
        raise ValueError("Approved art must be CPYBG12\\0 + 256x192 opaque RGB555LE")
    image=src[8:]
    if any(not (value & 0x8000) for (value,) in struct.iter_unpack("<H",image)):
        raise ValueError("Art has a transparent RGB555 pixel")
    if len(raw)<0x200:
        raise ValueError("Not an NDS ROM")
    off=struct.unpack_from("<I",raw,0x20)[0]
    size=struct.unpack_from("<I",raw,0x2c)[0]
    if off < 0x200 or size < len(WAIT)+len(image) or off+size>len(raw):
        raise ValueError("Invalid ARM9 ROM section bounds")
    arm9=raw[off:off+size]
    count=arm9.count(WAIT)
    if count!=1:
        raise ValueError(f"Expected exactly one unpatched embedded image; found {count}")
    relative=arm9.index(WAIT)
    marker_pos=off+relative
    start=marker_pos+len(WAIT)
    if raw[start:start+len(image)]!=PLACEHOLDER:
        raise ValueError("Reserved RGB555 storage differs from expected placeholder")
    patched=bytearray(raw)
    patched[marker_pos:start]=READY
    patched[start:start+len(image)]=image
    if len(patched)!=len(raw):
        raise AssertionError("ROM size changed unexpectedly")
    dest.parent.mkdir(parents=True,exist_ok=True)
    dest.write_bytes(patched)
    verify=dest.read_bytes()
    if verify[marker_pos:start]!=READY or verify[start:start+len(image)]!=image:
        raise AssertionError("Embedded artwork failed byte-identical readback")
    if verify[:0x200]!=raw[:0x200]:
        raise AssertionError("ROM header modified")
    return {
        "rom":str(dest),"rom_bytes":len(verify),
        "image_bytes":len(image),
        "image_sha256":hashlib.sha256(image).hexdigest(),
        "rom_sha256":hashlib.sha256(verify).hexdigest(),
        "internal_offset":marker_pos,
        "single_nds":True
    }

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom",type=Path,required=True)
    parser.add_argument("--art",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    print(embed(args.rom,args.art,args.output))
