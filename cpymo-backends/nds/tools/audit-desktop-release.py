#!/usr/bin/env python3
"""Audit four same-commit CPyMO for NDS release distributions.

Read the actual ZIPs, check paired ROM, corresponding FFmpeg
source, the Windows PE icon, Mac Finder icon, and required artwork attribution.
Never treat a green individual build job as cross-package acceptance.
Pillow reconstructs the pinned Mac derivation and checks every actual ICNS frame.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import plistlib
import struct
import zipfile

APPROVED_ICON_SOURCE_SHA256 = "133abae11b6d958c8e02d70e67dcf997049b5277bde3c734860e19f80257d90e"
APPROVED_ICON_RGBA_SHA256 = "a5954aa3fb04caecaf3796a314910f1e6fcc6d62d72d2b07a582f50ec15a4460"
APPROVED_ICON_ID = "approved-pymo-original-rounded-2026"
MACOS_CONTENT_SCALE = 0.80
APPROVED_FFMPEG_SOURCE_SHA256 = "de668509caf9e35e3cd162473441fdb29538c6d96ed080292b3cf9e6fc5d558f"
VERSION = "1.3.0"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_zip(path: Path) -> zipfile.ZipFile:
    z = zipfile.ZipFile(path)
    damaged = z.testzip()
    if damaged is not None:
        raise ValueError(f"Corrupt member in {path}: {damaged}")
    return z


def verify_macos_icon(source_png: bytes, manifest: dict, icns: bytes) -> dict:
    """Independently reconstruct the approved Mac derivation, including every frame."""
    from PIL import Image
    assert digest(source_png) == APPROVED_ICON_SOURCE_SHA256
    with Image.open(io.BytesIO(source_png)) as opened:
        source = opened.convert("RGBA")
    assert source.size == (1024, 1024)
    assert digest(source.tobytes()) == APPROVED_ICON_RGBA_SHA256
    side = round(1024 * MACOS_CONTENT_SCALE)
    offset = (1024 - side) // 2
    expected = Image.new("RGBA", (1024, 1024), (0, 0, 0, 0))
    expected.alpha_composite(source.resize((side, side), Image.Resampling.LANCZOS), (offset, offset))
    rule = manifest["macos_derivation"]
    assert rule == {"content_scale": MACOS_CONTENT_SCALE, "canvas_size": [1024, 1024],
                    "content_size": [side, side], "offset": [offset, offset],
                    "padding": [offset, offset, 1024-side-offset, 1024-side-offset],
                    "resampling": "LANCZOS", "uniform_scale": True,
                    "crop_or_corner_mask_applied": False,
                    "derived_rgba_sha256": digest(expected.tobytes())}
    assert digest(icns) == manifest["outputs"]["icns"]["sha256"]
    frames = {}
    with Image.open(io.BytesIO(icns)) as icon:
        assert icon.size == (1024, 1024)
        for size in icon.info["sizes"]:
            frame = icon.icns.getimage(size).convert("RGBA")
            required = expected.resize(frame.size, Image.Resampling.LANCZOS)
            assert frame.tobytes() == required.tobytes(), ("Incorrect Mac ICNS frame", size)
            frames[f"{size[0]}x{size[1]}@{size[2]}"] = digest(frame.tobytes())
    return {"status": "PASS", "content_scale": MACOS_CONTENT_SCALE,
            "content_size": [side, side], "offset": [offset, offset],
            "derived_rgba_sha256": digest(expected.tobytes()), "frames_rgba_sha256": frames}


def member(z: zipfile.ZipFile, suffix: str) -> str:
    matches = [p for p in z.namelist() if p.endswith("/" + suffix)]
    if len(matches) != 1:
        raise ValueError(f"Expected one {suffix}, got {matches}")
    return matches[0]


def ensure_pe_icon(payload: bytes, expected_frames: dict | None = None) -> dict:
    """Check the actual .exe resources, not just PyInstaller's --icon CLI."""
    u16 = lambda i: struct.unpack_from("<H", payload, i)[0]
    u32 = lambda i: struct.unpack_from("<I", payload, i)[0]
    assert payload[:2] == b"MZ"
    pe = u32(0x3c)
    assert payload[pe:pe + 4] == b"PE\x00\x00"
    coff = pe + 4
    optional = coff + 20
    nsec, opt_size = u16(coff + 2), u16(coff + 16)
    dir_base = optional + (112 if u16(optional) == 0x20b else 96)
    resource_rva = u32(dir_base + 16)
    sections = []
    for j in range(nsec):
        o = optional + opt_size + j * 40
        sections.append((u32(o+12), u32(o+8), u32(o+20), u32(o+16)))

    def offset(rva):
        for virtual, size, raw, raw_size in sections:
            if virtual <= rva < virtual + max(size, raw_size):
                return raw + rva - virtual
        raise ValueError(f"PE resource RVA out of bounds: {rva:#x}")

    base = offset(resource_rva)

    def entries(rel):
        o = base + rel
        count = u16(o + 12) + u16(o + 14)
        return [(u32(o + 16 + i * 8), u32(o + 20 + i * 8))
                for i in range(count)]

    root = entries(0)
    def child(typ):
        candidates = [rel for key, rel in root if key == typ and rel & 0x80000000]
        assert len(candidates) == 1, f"Missing PE icon resource {typ}"
        return entries(candidates[0] & 0x7fffffff)

    images = child(3)   # RT_ICON
    groups = child(14)  # RT_GROUP_ICON
    assert len(images) >= 7 and len(groups) == 1
    lang_entries = entries(groups[0][1] & 0x7fffffff)
    assert lang_entries
    _, leaf = lang_entries[0]
    assert not leaf & 0x80000000
    data_rva = u32(base + leaf)
    size = u32(base + leaf + 4)
    data = payload[offset(data_rva):offset(data_rva)+size]
    reserved, kind, count = struct.unpack_from("<HHH", data)
    assert (reserved, kind, count) == (0, 1, 7)
    sizes = set()
    frame_hashes = {}
    for i in range(count):
        w, h, _, _, _, bpp, image_bytes, image_id = struct.unpack_from("<BBBBHHIH", data, 6+i*14)
        assert bpp == 32
        sizes.add((w or 256, h or 256))
        candidates=[rel for key,rel in images if key==image_id]
        assert len(candidates)==1 and candidates[0]&0x80000000
        translations=entries(candidates[0]&0x7fffffff)
        assert translations
        hashes=[]
        for _,image_leaf in translations:
            assert not image_leaf&0x80000000
            rva=u32(base+image_leaf);length=u32(base+image_leaf+4)
            assert length==image_bytes
            hashes.append(digest(payload[offset(rva):offset(rva)+length]))
        assert len(set(hashes))==1
        frame_hashes[f"{w or 256}x{h or 256}"]=hashes[0]
    assert {(16,16), (24,24), (32,32), (48,48), (64,64), (128,128), (256,256)} <= sizes
    if expected_frames is not None:
        assert frame_hashes==expected_frames, "PE contains wrong icon image bytes"
    return {"PE_group_icon": "PASS", "sizes": sorted(sizes),
            "frame_payloads_sha256":frame_hashes,
            "image_payloads_match_manifest":expected_frames is not None}


def main():
    ap = argparse.ArgumentParser()
    for name in ("rom", "windows", "macos", "sources"):
        ap.add_argument("--" + name, type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args()
    paths = {name: getattr(args, name) for name in ("rom", "windows", "macos", "sources")}
    for path in paths.values():
        if not path.is_file():
            ap.error(f"Missing paired distribution {path}")
    rom = args.rom.read_bytes()
    # Version-specific ROM length and hash are discovered from THIS build.
    # The shared release verifier independently checks NTR/TWL headers and
    # exact owner-approved dual-screen background; placeholder ROMs fail.
    verifier_path = Path(__file__).with_name("verify-release-artifact.py")
    spec = importlib.util.spec_from_file_location("nds_release_verifier", verifier_path)
    assert spec and spec.loader
    verifier = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(verifier)
    verified_rom = verifier.validate_rom_bytes(rom)
    assert verified_rom["single_rom_for_NTR_TWL"]
    icon_path = Path(__file__).with_name("verify_nds_icons.py")
    icon_spec = importlib.util.spec_from_file_location("nds_icon_verifier", icon_path)
    assert icon_spec and icon_spec.loader
    icon_verifier = importlib.util.module_from_spec(icon_spec)
    icon_spec.loader.exec_module(icon_verifier)
    nds_root = Path(__file__).resolve().parents[1]
    verified_icons = icon_verifier.verify(nds_root, args.rom)
    report = {"status": "Same-commit release assets verified",
              "source_commit": os.environ.get("GITHUB_SHA", "unknown"),
              "workflow_run": os.environ.get("GITHUB_RUN_ID", "local"),
              "rom_sha256": digest(rom), "verified_rom": verified_rom,
              "nds_icons": verified_icons,
              "distributions": {}}
    zip_reports = {}
    with read_zip(paths["sources"]) as source_zip:
        approved_source_png = source_zip.read(member(source_zip, "cpymo-tool/nds-converter/assets/converter-icon-source.png"))
    ffmpeg_sources = []
    for kind in ("windows", "macos"):
        with read_zip(paths[kind]) as z:
            if kind == "macos":
                mangled = [name for name in z.namelist() if not name.isascii()]
                assert not mangled, ("macOS ZIP names must be ASCII to avoid ditto mojibake", mangled)
            info = {"bytes": paths[kind].stat().st_size,
                    "sha256": digest(paths[kind].read_bytes())}
            roms = [p for p in z.namelist() if p.endswith("/cpymo-nds.nds")
                    and z.getinfo(p).file_size == len(rom)]
            assert len(roms) == 2, (kind, roms)
            assert all(digest(z.read(n)) == digest(rom) for n in roms)
            metadata = json.loads(z.read(member(z, "VERSION.json")))
            assert metadata["version"] == VERSION
            assert metadata["rom_sha256"] == digest(rom)
            assert metadata["desktop_icon"] == APPROVED_ICON_ID
            assert metadata["desktop_icon_source_sha256"] == APPROVED_ICON_SOURCE_SHA256
            assert metadata["desktop_icon_pixel_sha256"] == APPROVED_ICON_RGBA_SHA256
            icon_manifest=json.loads(z.read(member(z,"ICON-MANIFEST.json")))
            assert icon_manifest["artwork_id"] == APPROVED_ICON_ID
            assert icon_manifest["source_sha256"] == APPROVED_ICON_SOURCE_SHA256
            assert icon_manifest["source_rgba_sha256"] == APPROVED_ICON_RGBA_SHA256
            assert icon_manifest["optical_width_scale"] == 1.0
            assert icon_manifest["reapplied_crop_or_mask"] is False
            info["desktop_artwork_id"]=APPROVED_ICON_ID
            # Windows Compress-Archive preserves the Chinese filename.
            # macOS ditto may mangle UTF-8 entry names; ship ASCII filenames there.
            label = "图标来源与署名.txt" if kind == "windows" else "ICON-ATTRIBUTION.txt"
            for label in (label, "PYMO-ORIGINAL-MIT-LICENSE.txt"):
                contents = z.read(member(z, label))
                assert len(contents) > 100 and b"pymo" in contents.lower(), label
            src = z.read(member(z, "FFmpeg-Corresponding-Source/ffmpeg-7.1.5.tar.xz"))
            assert digest(src) == APPROVED_FFMPEG_SOURCE_SHA256
            ffmpeg_sources.append(digest(src))
            conf = z.read(member(z, "FFmpeg-Corresponding-Source/FFMPEG-BUILDCONF.txt")).decode()
            assert "--disable-gpl" in conf and "--disable-nonfree" in conf
            assert "--enable-gpl" not in conf and "--enable-nonfree" not in conf
            info["rom_matches"] = True
            info["pymo_icon_license_and_attribution"] = "present"
            info["ffmpeg_corresponding_source_sha256"] = digest(src)
            if kind == "windows":
                exe = z.read(member(z, "Converter/Converter.exe"))
                info["windows_icon"] = ensure_pe_icon(exe,icon_manifest["windows_ico_frames_sha256"])
            else:
                plist = plistlib.loads(z.read(member(z, "CPyMO-NDS-Converter.app/Contents/Info.plist")))
                assert plist.get("CFBundleVersion") == VERSION
                assert plist.get("CFBundleShortVersionString") == VERSION
                assert plist.get("CFBundleIconFile") == "converter-icon.icns"
                icns = z.read(member(z, "Contents/Resources/converter-icon.icns"))
                assert icns[:4] == b"icns" and len(icns) > 100000
                assert digest(icns)==icon_manifest["outputs"]["icns"]["sha256"]
                derivation = verify_macos_icon(approved_source_png, icon_manifest, icns)
                assert metadata.get("macos_dock_icon_source") == "bundle_icns"
                assert metadata.get("macos_icon_derivation") == icon_manifest["macos_derivation"]
                assert metadata.get("macos_architecture") == "arm64"
                assert metadata.get("rosetta_required") is False
                info["macos_icon"] = {"sha256": digest(icns), "plist": "MATCH",
                                      "derivation": derivation, "runtime_source": "bundle_icns"}
                info["native_arch"] = "arm64"
            report["distributions"][kind] = info
    assert len(set(ffmpeg_sources)) == 1
    with read_zip(paths["sources"]) as z:
        source_icon = z.read(member(z, "cpymo-tool/nds-converter/assets/converter-icon-source.png"))
        assert digest(source_icon) == APPROVED_ICON_SOURCE_SHA256
        generator = z.read(member(z, "cpymo-tool/nds-converter/tools/build_desktop_icons.py"))
        assert APPROVED_ICON_SOURCE_SHA256.encode() in generator
        assert APPROVED_ICON_RGBA_SHA256.encode() in generator
        # The source archive carries the exact approved lossless rounded PNG;
        # all desktop derivatives are reproducible without the retired artwork.
        for name in ("ICON-ATTRIBUTION.txt", "PYMO-ORIGINAL-MIT-LICENSE.txt"):
            assert z.getinfo(member(z, "cpymo-tool/nds-converter/assets/" + name)).file_size > 100
        assert z.getinfo(f"CPyMO-for-NDS-{VERSION}-Sources/LICENSE").file_size > 100
        nds_manifest = json.loads((nds_root / 'assets/NDS-ICON-MANIFEST.json').read_text())
        nds_files = ['assets/' + nds_manifest['source'], 'assets/NDS-ICON-MANIFEST.json',
                     'tools/build_nds_icons.py', 'tools/verify_nds_icons.py',
                     *nds_manifest['outputs']]
        for relative in nds_files:
            assert z.read(member(z, 'cpymo-backends/nds/' + relative)) == (nds_root / relative).read_bytes(), relative
    report["distributions"]["sources"] = {"bytes": paths["sources"].stat().st_size,
                                           "sha256": digest(paths["sources"].read_bytes()),
                                           "approved_icon_source_sha256": digest(source_icon)}
    report["distributions"]["rom"] = {"bytes": len(rom), "sha256": digest(rom)}
    args.output.mkdir(parents=True, exist_ok=True)
    output = args.output / f"CPyMO-for-NDS-{VERSION}-SHA256SUMS.txt"
    output.write_text("".join(f"{digest(paths[name].read_bytes())}  {paths[name].name}\n"
                              for name in ("rom","windows","macos","sources")), encoding="ascii")
    (args.output / "RELEASE-PAIRED-AUDIT.json").write_text(
        json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
