#!/usr/bin/env python3
"""Compile the REAL NDS enhanced lower-canvas C function and preview its RGB555 output.

Each preview is only the exact 256x192 canvas/backdrop drawn BEFORE real PyMO
game-owned dialogue, text, menu sprites and launcher hint glyphs. This is NOT
a game screenshot, emulator screenshot, or AI illustration.

Requires a host C compiler (gcc/clang), no NDS SDK. PNG output uses only the
source-identical RGB555 converter already included in this repository.
"""
from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
SRC = ROOT / "cpymo-backends/nds/source"
CONVERTER = SRC.parent / "tools/nds-capture-to-png.py"
spec = importlib.util.spec_from_file_location("nds_capture_to_png", CONVERTER)
capture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(capture)

C_SOURCE = r"""
#include "dual-canvas-nds.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static uint16_t buffer[256*192];
int main(int argc,char**argv){
 if(argc!=3)return 2;
 int interactive=atoi(argv[1]);
 int dialogue=atoi(argv[2]);
 if(interactive<0||interactive>1||dialogue<0||dialogue>1)return 3;
 nds_dual_canvas_draw(buffer,interactive,dialogue);
 if(fwrite(buffer,sizeof buffer[0],256*192,stdout)!=256*192)return 4;
 return 0;
}
"""

def convert_one(raw: bytes) -> bytes:
    # Reuse exact RGB555 -> native PNG conversion, with no image editing.
    if len(raw) != capture.FRAME_BYTES:
        raise ValueError(f"Bad native framebuffer size: {len(raw)}")
    top = bytes(capture.FRAME_BYTES)
    header = capture.HEADER.pack(capture.MAGIC, 256, 192, 2, 1)
    _, (_unused_top, rgb) = capture.decode_capture(header + top + raw)
    return capture.png_rgb(256, 192, rgb)

def render(output: Path) -> list[Path]:
    output.mkdir(parents=True, exist_ok=True)
    cases = (("01-dialogue-canvas",0,1),
             ("02-menu-canvas",1,0),
             ("03-idle-canvas",0,0))
    results = []
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp)
        test_c = path / "render.c"
        test_c.write_text(C_SOURCE, encoding="utf-8")
        binary = path / "render"
        subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                        "-I",str(SRC),str(test_c),"-o",str(binary)],
                       capture_output=True,text=True,check=True)
        for name, interactive, dialogue in cases:
            raw = subprocess.run([str(binary),str(interactive),str(dialogue)],
                                 capture_output=True,check=True).stdout
            png = output / f"{name}-256x192.png"
            png.write_bytes(convert_one(raw))
            results.append(png)
    return results

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("dual-canvas-output"))
    args = parser.parse_args()
    for result in render(args.output):
        print(result)
