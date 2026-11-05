#!/usr/bin/env python3
"""Build Windows/macOS companion-converter icons from the approved rounded PNG.

The 1024x1024 source is already cropped and antialiased. Do not apply the old
16px outline erosion, a second corner mask, or horizontal optical compression.
The NDS game's full-square banner and launcher resources are independent.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
from PIL import Image

ARTWORK_ID = "approved-pymo-original-rounded-2026"
SOURCE_SHA256 = "133abae11b6d958c8e02d70e67dcf997049b5277bde3c734860e19f80257d90e"
APPROVED_RGBA_SHA256 = "a5954aa3fb04caecaf3796a314910f1e6fcc6d62d72d2b07a582f50ec15a4460"
SOURCE_SIZE = (1024, 1024)
WINDOWS_SIZES = ((16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256))
OPTICAL_WIDTH_SCALE = 1.0  # Retained for compatibility; never alter the new art.
MACOS_CONTENT_SCALE = 0.80

def macos_canvas(image: Image.Image) -> tuple[Image.Image, dict]:
    """Scale the whole approved artwork uniformly onto a transparent Mac canvas."""
    side = round(SOURCE_SIZE[0] * MACOS_CONTENT_SCALE)
    offset = (SOURCE_SIZE[0] - side) // 2
    canvas = Image.new("RGBA", SOURCE_SIZE, (0, 0, 0, 0))
    canvas.alpha_composite(image.convert("RGBA").resize((side, side), Image.Resampling.LANCZOS),
                           (offset, offset))
    rule = {"content_scale": MACOS_CONTENT_SCALE, "canvas_size": list(SOURCE_SIZE),
            "content_size": [side, side], "offset": [offset, offset],
            "padding": [offset, offset, SOURCE_SIZE[0]-side-offset, SOURCE_SIZE[1]-side-offset],
            "resampling": "LANCZOS", "uniform_scale": True,
            "crop_or_corner_mask_applied": False, "derived_rgba_sha256": pixel_sha(canvas)}
    return canvas, rule

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def pixel_sha(image: Image.Image) -> str:
    return hashlib.sha256(image.convert("RGBA").tobytes()).hexdigest()

def ico_frame_hashes(path: Path) -> dict[str, str]:
    """Hash real RT_ICON payloads so the release audit can inspect the EXE."""
    data=path.read_bytes()
    if len(data)<6 or struct.unpack_from("<HH",data)!=(0,1):
        raise ValueError("Invalid generated Windows ICO header")
    count=struct.unpack_from("<H",data,4)[0]
    result={}
    for i in range(count):
        w,h,_,_,_,_,size,offset=struct.unpack_from("<BBBBHHII",data,6+i*16)
        if size<=0 or offset+size>len(data):
            raise ValueError("Generated ICO frame is out of bounds")
        key=f"{w or 256}x{h or 256}"
        if key in result:
            raise ValueError("Duplicate ICO frame: "+key)
        result[key]=hashlib.sha256(data[offset:offset+size]).hexdigest()
    return result

def build(source: Path, dest: Path, *, allow_alternate_source: bool=False) -> dict:
    source=Path(source);dest=Path(dest)
    if not source.is_file():
        raise FileNotFoundError(f"Approved icon source missing: {source}")
    source_hash=sha(source)
    if not allow_alternate_source and source_hash!=SOURCE_SHA256:
        raise ValueError("Wrong desktop icon source; expected the approved "
                         "1024px rounded original, not the retired 04 artwork: "+source_hash)
    with Image.open(source) as opened:
        if opened.format!="PNG":
            raise ValueError("Use the lossless approved PNG, not a lossy substitute")
        image=opened.convert("RGBA")
    if image.size!=SOURCE_SIZE:
        raise ValueError(f"Expected {SOURCE_SIZE}, got {image.size}")
    if image.getchannel("A").getextrema()[0]>=255:
        raise ValueError("Expected transparent antialiased corners")
    rgba_hash=pixel_sha(image)
    if not allow_alternate_source and rgba_hash!=APPROVED_RGBA_SHA256:
        raise ValueError("Approved source's decoded pixels differ: "+rgba_hash)
    dest.mkdir(parents=True,exist_ok=True)
    paths={"png":dest/"converter-icon.png", "png_64":dest/"converter-icon-64.png",
           "macos_png":dest/"converter-icon-macos.png",
           "ico":dest/"converter-icon.ico", "icns":dest/"converter-icon.icns"}
    if source.resolve()==paths["png"].resolve():
        raise ValueError("Do not generate output over the canonical source")
    # Preserve full-size PNG bytes exactly, including its approved alpha mask.
    shutil.copyfile(source,paths["png"])
    image.resize((64,64),Image.Resampling.LANCZOS).save(paths["png_64"],format="PNG",optimize=True)
    image.save(paths["ico"],format="ICO",sizes=list(WINDOWS_SIZES))
    mac_image, mac_rule = macos_canvas(image)
    mac_image.save(paths["macos_png"],format="PNG",optimize=True)
    # Pillow's ICNS default resize uses BICUBIC; provide explicit frames so
    # every stored resolution follows the recorded LANCZOS derivation.
    mac_image.save(paths["icns"],format="ICNS",append_images=[
        mac_image.resize((n,n),Image.Resampling.LANCZOS) for n in (32,64,128,256,512,1024)])
    with Image.open(paths["ico"]) as check:
        if set(check.info["sizes"])!=set(WINDOWS_SIZES):
            raise ValueError("Windows ICO resolution inventory mismatch")
    with Image.open(paths["icns"]) as check:
        if check.size!=SOURCE_SIZE or pixel_sha(check)!=mac_rule["derived_rgba_sha256"]:
            raise ValueError("ICNS largest frame must match the approved Mac scale and transparent padding")
    manifest={
        "artwork_id":ARTWORK_ID,
        "source":"converter-icon-source.png",
        "source_sha256":source_hash,
        "source_rgba_sha256":rgba_hash,
        "source_size":list(SOURCE_SIZE),
        "macos_derivation":mac_rule,
        "optical_width_scale":1.0,
        "reapplied_crop_or_mask":False,
        "lossy_reencoding":False,
        "nds_icon_modified":False,
        "source_attribution":"assets/ICON-ATTRIBUTION.txt",
        "outputs":{k:{"file":p.name,"sha256":sha(p),"bytes":p.stat().st_size}
                   for k,p in paths.items()},
        "windows_ico_frames_sha256":ico_frame_hashes(paths["ico"]),
    }
    (dest/"icon-manifest.json").write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
    return manifest

def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--allow-alternate-source",action="store_true",
                   help="Synthetic development fixtures only, never public packages")
    a=p.parse_args()
    print(json.dumps(build(a.source,a.output,allow_alternate_source=a.allow_alternate_source),
                     ensure_ascii=False,indent=2))

if __name__=="__main__":
    main()
