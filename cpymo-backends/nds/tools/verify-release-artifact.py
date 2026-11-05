#!/usr/bin/env python3
"""Read-only gate for CPyMO for NDS release artifacts.

Validates the *actual published ROM*, not an intermediate SDK build. The
unpatched/placeholder build must FAIL. Also check any paired Windows/macOS
archives have the exact same ROM bytes. This does not replace hardware tests,
GUI testing, FFmpeg licensing, or full corresponding-source review.
"""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import io
import json
import stat
from pathlib import Path
import struct
import zipfile

READY = b"CPYMO_BG_READY12"
EXPECTED_BG_SHA256 = "92d91514ac12ccab6037a86a71e0c4cf1047917d15de59b91bcc8e4bb4d7d1b2"
FRAME_BYTES = 256 * 192 * 2
BACKEND = Path(__file__).resolve().parents[1]
EXPECTED_VERSION = json.loads((BACKEND / "release.json").read_text(encoding="utf8"))["version"]
_banner_spec = importlib.util.spec_from_file_location("release_rom_banner", BACKEND / "rom_banner.py")
_banner_module = importlib.util.module_from_spec(_banner_spec)
_banner_spec.loader.exec_module(_banner_module)


def sha(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def validate_rom_bytes(b: bytes) -> dict:
    if len(b) < 0x200:
        raise ValueError("Not a valid Nintendo DS ROM header")
    if b[0x12] != 2:
        raise ValueError("ROM header does not advertise DS/DSi compatibility")
    offset, size = struct.unpack_from("<I", b, 0x20)[0], struct.unpack_from("<I", b, 0x2c)[0]
    if offset < 0x200 or size < FRAME_BYTES or offset + size > len(b):
        raise ValueError("Invalid ARM9 ROM bounds")
    arm9 = b[offset:offset + size]
    # Check compiled ARM9 text independently of the outer filename and Banner.
    runtime_title = f"CPyMO for NDS   {EXPECTED_VERSION}".encode("ascii")
    if runtime_title not in arm9:
        raise ValueError(f"Compiled runtime version is not {EXPECTED_VERSION}")
    _, banner = _banner_module._load_banner(io.BytesIO(b))
    titles = []
    for i in range(_banner_module.LANGUAGE_COUNT):
        start = _banner_module.TITLE_START + i * _banner_module.TITLE_SLOT_BYTES
        text = banner[start:start + _banner_module.TITLE_SLOT_BYTES].decode("utf-16le")
        titles.append(text.split("\x00", 1)[0].split("\n"))
    if any(title != ["CPyMO for NDS", EXPECTED_VERSION, "千岚之夏"] for title in titles):
        raise ValueError(f"Banner version/title is not {EXPECTED_VERSION}")
    found = []
    i = 0
    while True:
        pos = arm9.find(READY, i)
        if pos < 0:
            break
        start = pos + len(READY)
        frame = arm9[start:start + FRAME_BYTES]
        if len(frame) == FRAME_BYTES and sha(frame) == EXPECTED_BG_SHA256:
            found.append(offset + pos)
            if any((value & 0x8000) == 0 for (value,) in struct.iter_unpack("<H", frame)):
                raise ValueError("Approved background has transparent pixels")
        i = pos + len(READY)
    if len(found) != 1:
        raise ValueError(
            f"Approved 02-B background not found exactly once ({len(found)} matches); "
            "do not publish the placeholder-only CI ROM"
        )
    return {"rom_bytes": len(b), "rom_sha256": sha(b),
            "release_version": EXPECTED_VERSION, "compiled_runtime_version": EXPECTED_VERSION,
            "banner_titles": titles, "banner_crc16": "PASS",
            "runtime_header_unit": b[0x12], "approved_art_sha256": EXPECTED_BG_SHA256,
            "approved_art_offset": found[0], "single_rom_for_NTR_TWL": True}


def verify_archive(path: Path, expected_sha: str) -> dict:
    names = []
    with zipfile.ZipFile(path) as z:
        for item in z.infolist():
            if item.is_dir() or not item.filename.lower().endswith(".nds"):
                continue
            # macOS PyInstaller applications include symlink entries to the
            # actual Contents/Resources ROM. ZIP stores a symlink's link text,
            # not the image bytes; do not mistake that text for a second ROM.
            # The real bundled ROM is validated during macOS app construction.
            if stat.S_ISLNK(item.external_attr >> 16):
                continue
            # Starter bundles include one copy for players and another inside
            # the frozen worker. All such copies must be byte-identical.
            h = sha(z.read(item))
            if h != expected_sha:
                raise ValueError(
                    f"{path.name}: {item.filename} differs from officially validated ROM "
                    f"({h} vs {expected_sha})"
                )
            names.append(item.filename)
        if z.testzip() is not None:
            raise ValueError(f"Archive corruption detected: {path.name}")
    if not names:
        raise ValueError(f"{path.name}: no packaged .nds found")
    return {"file": path.name, "size_bytes": path.stat().st_size,
            "rom_copies": names, "rom_sha256": expected_sha}


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--rom", type=Path, required=True)
    p.add_argument("--windows-starter", type=Path)
    p.add_argument("--macos-starter", type=Path)
    p.add_argument("--report", type=Path)
    args = p.parse_args()
    raw = args.rom.read_bytes()
    result = validate_rom_bytes(raw)
    result["rom_file"] = args.rom.name
    result["paired_archives"] = []
    for archive in (args.windows_starter, args.macos_starter):
        if archive:
            result["paired_archives"].append(
                verify_archive(archive, result["rom_sha256"])
            )
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, ensure_ascii=True))


if __name__ == "__main__":
    main()
