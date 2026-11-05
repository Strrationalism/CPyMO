#!/usr/bin/env python3
"""Encode an owner-approved 256x192 PNG into CPyMO's native lower-LCD art.

This is a build/asset-preparation helper, NOT a game-time PNG decoder.
Runtime reads exactly 8-byte CPYBG12\\0 + 49152 RGB555LE pixels.
Requires Pillow on the host only.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct

def encode(src: Path, dest: Path) -> dict:
    from PIL import Image
    with Image.open(src) as image:
        if image.size != (256,192):
            raise ValueError("Expected native 256x192 RGB PNG")
        rgb=image.convert("RGB")
        raw=bytearray(b"CPYBG12\0")
        for r,g,b in rgb.getdata():
            native=(0x8000 | ((r*31+127)//255) |
                    (((g*31+127)//255)<<5) | (((b*31+127)//255)<<10))
            raw+=struct.pack("<H",native)
    assert len(raw)==8+256*192*2
    dest.parent.mkdir(parents=True,exist_ok=True)
    dest.write_bytes(raw)
    return {"path":str(dest),"bytes":len(raw),
            "sha256":hashlib.sha256(raw).hexdigest()}

if __name__=="__main__":
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("source_png",type=Path)
    ap.add_argument("output_rgb555",type=Path)
    args=ap.parse_args()
    print(encode(args.source_png,args.output_rgb555))
