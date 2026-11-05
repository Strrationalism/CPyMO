#!/usr/bin/env python3
"""Convert REAL CPyMO NDS/DSi LCD captures to pixel-exact PNGs.

Input is emitted by the 1.2 experimental .nds through the optional flag
`save/nds-visual-capture.flag`. This converter does NOT draw artwork, mimic
an emulator, call image generation or fabricate any visual-novel screenshot.

Output is exactly 256 x 192 pixels per LCD, with no resampling.
Standard-library only (struct/zlib); works without Pillow/ffmpeg.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import struct
import zlib

MAGIC = b"NDSPIX1\0"
WIDTH, HEIGHT = 256, 192
HEADER = struct.Struct("<8s4H")
FRAME_BYTES = WIDTH * HEIGHT * 2
EXPECTED_BYTES = HEADER.size + 2 * FRAME_BYTES


def decode_capture(data: bytes) -> tuple[int, tuple[bytes, bytes]]:
    if len(data) != EXPECTED_BYTES:
        raise ValueError(f"Invalid capture length: {len(data)} (expected {EXPECTED_BYTES})")
    magic, width, height, frames, kind = HEADER.unpack_from(data)
    if magic != MAGIC or (width, height, frames) != (WIDTH, HEIGHT, 2):
        raise ValueError("Not an NDSPIX1 256x192 two-screen RGB555 capture")
    if kind not in (1, 2, 3, 4):
        raise ValueError("Unsupported display capture kind")
    result = []
    for start in (HEADER.size, HEADER.size + FRAME_BYTES):
        src = memoryview(data)[start : start + FRAME_BYTES]
        rgb = bytearray(WIDTH * HEIGHT * 3)
        for i in range(0, FRAME_BYTES, 2):
            px = src[i] | (src[i + 1] << 8)
            r, g, b = px & 31, (px >> 5) & 31, (px >> 10) & 31
            offset = i // 2 * 3
            rgb[offset] = r << 3 | r >> 2
            rgb[offset + 1] = g << 3 | g >> 2
            rgb[offset + 2] = b << 3 | b >> 2
        result.append(bytes(rgb))
    return kind, (result[0], result[1])


def png_chunk(tag: bytes, data: bytes) -> bytes:
    return (struct.pack(">I", len(data)) + tag + data +
            struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))


def png_rgb(width: int, height: int, rgb: bytes) -> bytes:
    if len(rgb) != width * height * 3:
        raise ValueError("Unexpected RGB buffer length")
    scanlines = bytearray()
    for y in range(height):
        scanlines.append(0)  # PNG filter: None
        scanlines.extend(rgb[y * width * 3 : (y + 1) * width * 3])
    return (b"\x89PNG\r\n\x1a\n" +
            png_chunk(b"IHDR", struct.pack(">2I5B", width, height, 8, 2, 0, 0, 0)) +
            png_chunk(b"IDAT", zlib.compress(bytes(scanlines), 9)) +
            png_chunk(b"IEND", b""))


def convert(path: Path) -> tuple[Path, Path]:
    _kind, (top, bottom) = decode_capture(path.read_bytes())
    paths = (path.with_name(path.stem + "-top.png"),
             path.with_name(path.stem + "-bottom.png"))
    for target, rgb in zip(paths, (top, bottom)):
        target.write_bytes(png_rgb(WIDTH, HEIGHT, rgb))
    return paths


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path, nargs="+",
                        help="Actual .ndsraw files copied from the game's save directory")
    args = parser.parse_args()
    for capture in args.capture:
        for path in convert(capture):
            print(f"Saved {path} ({WIDTH} x {HEIGHT}, native pixels)")


if __name__ == "__main__":
    main()
